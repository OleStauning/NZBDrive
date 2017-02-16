/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef NEWSCLIENTSOCKET_HPP
#define NEWSCLIENTSOCKET_HPP

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/error.hpp>
#include <atomic>
#include <set>
#include <mutex>
#include "Logger.hpp"

namespace ByteFountain
{

typedef boost::asio::io_service io_service;
typedef boost::asio::ssl::context ssl_context;
typedef boost::asio::ip::tcp::socket unsecure_socket;
typedef boost::asio::ssl::stream<unsecure_socket> secure_socket;
typedef boost::asio::ip::tcp::resolver tcp_resolver;
typedef boost::asio::ip::tcp::endpoint tcp_endpoint;
typedef boost::asio::ssl::stream_base ssl_streambase;
typedef boost::asio::io_service::work io_work;
typedef boost::asio::deadline_timer deadline_timer;

using namespace boost::posix_time;
using namespace boost::system;

class RateLimiter;

class RateLimiterTarget
{
	io_service& m_io_service;
	RateLimiter& m_limiter;
	std::size_t m_bytesAvailable;
	std::size_t m_bytesMissing;
	bool m_limitEnabled;
	std::function<void()> m_paused_handler;


	void Unpause();
public:
	RateLimiterTarget(io_service&,RateLimiter&);
	virtual ~RateLimiterTarget();

	std::size_t AddTokenBytes(const std::size_t bytes, const std::size_t max);
	void AddRXBytes(const std::size_t bytes, std::function<void()> handler);
	void AddTXBytes(const std::size_t bytes);
	void SetLimitEnabled(const bool enabled);
	void Stop();
};

struct NetworkStats
{
	std::atomic<std::size_t> RXBytes;
	std::atomic<std::size_t> TXBytes;
	
	NetworkStats():RXBytes(0),TXBytes(0){}
};


class RateLimiter
{
public:
	struct Parameters
	{
		std::size_t RXBytesPrS;
		std::size_t AdditionalTokens;
		std::size_t MaxBucketSize;
		Parameters():
			RXBytesPrS(0),
			AdditionalTokens(0),
			MaxBucketSize(0)
		{}
		Parameters(const std::size_t rxBytesPrS) :
			RXBytesPrS(rxBytesPrS),
			AdditionalTokens(0),
			MaxBucketSize(rxBytesPrS)
		{}
		Parameters(
			const std::size_t rxBytesPrS,
			const std::size_t additionalTokens
  			):
			RXBytesPrS(rxBytesPrS),
			AdditionalTokens(additionalTokens),
			MaxBucketSize(rxBytesPrS)
		{}
		Parameters(
			const std::size_t rxBytesPrS,
			const std::size_t additionalTokens,
			const std::size_t maxBucketSize
  			):
			RXBytesPrS(rxBytesPrS),
			AdditionalTokens(additionalTokens),
			MaxBucketSize(maxBucketSize)
		{}
	};
	
	io_service& m_io_service;
	Logger& m_logger;
	std::function<Parameters ()> m_configure;
	deadline_timer m_timer;
	bool m_stopping;
	bool m_timer_running;
	time_duration m_duration;
	std::set<RateLimiterTarget*> m_targets;
	std::mutex m_mutex;
	
	void SetNextTimerEvent();
	
	friend class RateLimiterTarget;
	void AddTokenBytes(const boost::system::error_code& e);
	void InsertTarget(RateLimiterTarget& ptarget);
	void RemoveTarget(RateLimiterTarget& ptarget);
//	bool Start();
	std::size_t AddTokens(const std::size_t tokens, const std::size_t maxTokens);
	void AddRXBytes(const std::size_t bytes);
	void AddTXBytes(const std::size_t bytes);
	
	NetworkStats m_network;
public:
	
	RateLimiter(io_service& io_service, Logger& logger, 
		std::function<Parameters()> configure = [](){ return RateLimiter::Parameters(0, 0, 0); });
	virtual ~RateLimiter();
	void Stop();

	const NetworkStats& Network;
};

class NewsClientSocket : public RateLimiterTarget
{
	io_service& m_io_service;
	bool m_ssl;
	ssl_context m_ssl_ctx;
	secure_socket m_ssl_socket;
	unsecure_socket m_tcp_socket;

public:
	NewsClientSocket(io_service& ios,  const bool ssl, RateLimiter& limiter):
		RateLimiterTarget(ios,limiter),
		m_io_service(ios),
		m_ssl(ssl), 
		m_ssl_ctx(m_io_service,ssl_context::sslv23),
		m_ssl_socket(m_io_service,m_ssl_ctx),
		m_tcp_socket(m_io_service)
	{
//		m_tcp_socket.set_option(boost::asio::socket_base::receive_buffer_size(10 * 1024));
	}
	
	~NewsClientSocket()
	{
//		close(); // for some reason, after hibernate, "close" caused:
	
//Warning: system  : Connection reset by peer (system:104)
//Warning: OnErrorRetry: segment 0 <part1of1.86f8SwrfeZlecN1SxGXC@camelsystem-powerpost.local>
// ..................	
// Debug: Client (1,0,0): Client restarting
// terminate called after throwing an instance of 'boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::system::system_error> >'
//  what():  shutdown: Transport endpoint is not connected
//Aborted (core dumped)

	
	}

	bool IsSSL() const { return m_ssl; } 
	
	template <typename T1, typename T2>
	void async_read_some(T1 mb, T2 h)
	{
		auto myh=[this,h](
			const boost::system::error_code& error,
			std::size_t bytes_transferred) mutable
		{
			AddRXBytes(bytes_transferred,
				[h, error, bytes_transferred]() mutable
				{
					h(error, bytes_transferred);
				}
			);
		};
		if (m_ssl) m_ssl_socket.async_read_some(mb,myh);
		else m_tcp_socket.async_read_some(mb,myh);
	}
	template <typename T1, typename T2>
	void async_write_some(T1 cb, T2 h)
	{
		auto myh=[this,h](
			const boost::system::error_code& error,
			std::size_t bytes_transferred) mutable
		{
			AddTXBytes(bytes_transferred);
			h(error, bytes_transferred);
		};
		if (m_ssl) m_ssl_socket.async_write_some(cb,myh); 
		else m_tcp_socket.async_write_some(cb,myh);
	}
	template <typename T1>
	size_t read_some(T1 mb)
	{
		if (m_ssl) return m_ssl_socket.read_some(mb); 
		else return m_tcp_socket.read_some(mb);
	}
	template <typename T1, typename T2>
	size_t read_some(T1 mb, T2 ec)
	{
		if (m_ssl) return m_ssl_socket.read_some(mb,ec); 
		else return m_tcp_socket.read_some(mb,ec);
	}
	template <typename T1>
	size_t write_some(T1 cb)
	{
		if (m_ssl) return m_ssl_socket.write_some(cb); 
		else return m_tcp_socket.write_some(cb);
	}
	template <typename T1, typename T2>
	size_t write_some(T1 cb, T2 ec)
	{
		if (m_ssl) return m_ssl_socket.write_some(cb,ec); 
		else return m_tcp_socket.write_some(cb,ec);
	}
	void close()
	{
		try
		{
			if (m_ssl && m_ssl_socket.lowest_layer().is_open())
			{
				m_ssl_socket.lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both);
				m_ssl_socket.lowest_layer().close();
			}
			if (m_tcp_socket.is_open())
			{
				m_tcp_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
				m_tcp_socket.close();
			}
		}
		catch (boost::exception &e)
		{
				// WHAT TO DO?
		}
	}
	bool is_open() const
	{
		if (m_ssl) return m_ssl_socket.lowest_layer().is_open(); 
		else return m_tcp_socket.is_open();
	}

	void async_connect(const tcp_endpoint& endpoint, const std::function<void(const boost::system::error_code& err)>& handler)
	{
		if (m_ssl)
		{
			m_ssl_socket.lowest_layer().async_connect(endpoint,handler);
		}
		else
		{
			m_tcp_socket.async_connect(endpoint,handler);
		}
	}

	void async_ssl_handshake(const std::function<void(const boost::system::error_code& err)>& handler)
	{
//		if (!m_ssl) throw std::system_error("Async handshake not applicable to non-ssl connection");
		m_ssl_socket.async_handshake(ssl_streambase::client, handler);
	}


};

	
}

#endif
