/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef NEWSCLIENT_HPP
#define NEWSCLIENT_HPP

#include <functional>
#include "NewsServer.hpp"
#include "NewsClientSocket.hpp"
#include <boost/algorithm/string.hpp>
#include <queue>
#include <signal.h>

#define ASIOMaxBufferSize 4096


namespace ByteFountain
{
class NewsClientCommand;

class NewsClient
{
	friend struct HandleCount;
public:
	enum DoneCode { OK, Error, RetryOther, ReconnectRetry, DisconnectClient };
	typedef std::function< void(NewsClient& client, NewsClientCommand& cmd, const DoneCode& code)> DoneHandler;

private:
	io_service& m_ios;
	Logger& m_logger;
	NewsServer& m_server;
	NewsClientSocket m_socket;
	DoneHandler m_done;
//	boost::asio::streambuf m_request;
	boost::asio::streambuf m_response;
	
	typedef std::pair<NewsClientCommand*, std::function<void(const boost::system::error_code& err, const std::size_t len)> > PipeCmd;
	std::queue< PipeCmd > m_pipeline;
	NewsClientCommand* m_active;
	boost::asio::deadline_timer m_timer;
	bool m_timeout;
	std::atomic<unsigned int> m_handles;
	bool m_disconnecting;
	
protected:
	virtual void OnStarted();
	virtual void OnStopped();
	
private:
	struct HandleCount
	{
		NewsClient& m_client;
		HandleCount(NewsClient& client) : m_client(client) 
		{ 
			if (m_client.m_handles++ == 0) m_client.OnStarted();
		}
		~HandleCount()
		{ 
			if (--m_client.m_handles == 0) m_client.OnStopped();
		}
		HandleCount& operator=(const HandleCount&) = delete;
		HandleCount(const HandleCount& other) : m_client(other.m_client){ ++m_client.m_handles; }
	};
	
public:

	NewsClient(
		io_service& ios
		, Logger& logger
		, RateLimiter& limiter
		, NewsServer& server
		, DoneHandler done
#ifndef RPI
			= [](NewsClient& client, NewsClientCommand& cmd, const DoneCode& code){}
#endif
		) :
		m_ios(ios),
		m_logger(logger),
		m_server(server),
		m_socket(ios, server.SSL, limiter),
		m_done(done),
		m_response(ASIOMaxBufferSize),
		m_pipeline(),
		m_active(0),
		m_timer(ios),
		m_timeout(false),
		m_handles(0),
		m_disconnecting(false)
	{}

	virtual ~NewsClient()
	{
        if (m_handles>0)
        {
            m_logger << Logger::Error << "~NewsClient has been called with " << m_handles << " handles" << Logger::End;
            raise (SIGABRT);
        }
        
		// THE FOLLOWING close() HAS BEEN REMOVED SINCE WAKEUP AFTER HIBERNATION CAUSES close TO THROW THE FOLLOWING EXCEPTION:
		// terminate called after throwing an instance of 'boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::system::system_error> >'
		// what():  shutdown: Transport endpoint is not connected
		m_socket.close();
		
		m_timer.cancel();
	}
	
	const unsigned int Handlers() const { return m_handles; }
	const NewsServer& GetServer() const { return m_server; }
	void Disconnect();
	bool Disconnecting() const { return m_disconnecting; }
	bool IsConnected() const { return m_socket.is_open(); }
	bool TimeoutOccurred() const { return m_timeout; } 
		
	bool ReadStatusResponse(unsigned int& status_code, std::string& message);
	const char* Data() { return boost::asio::buffer_cast<const char *>(m_response.data()); }
	void Consume(const std::size_t size) { m_response.consume(size); }
	
	void AsyncWriteRequest(NewsClientCommand& cmd, const std::function<void(const boost::system::error_code& err, const std::size_t len)>& func);
	void AsyncReadResponse(NewsClientCommand& cmd, const std::function<void(const boost::system::error_code& err, const std::size_t len)>& func);
	void AsyncReadDataResponse(NewsClientCommand& cmd, const std::function<void(const boost::system::error_code& err, const std::size_t len)>& func);

	void AsyncResolve(NewsClientCommand& cmd, tcp_resolver& resolver, const tcp_resolver::query& query,
		std::function<void(const boost::system::error_code& err, tcp_resolver::iterator endpoint_iterator)> handler);
	void AsyncConnect(NewsClientCommand& cmd, const tcp_endpoint& endpoint, std::function<void(const boost::system::error_code& err)> handler);
	void AsyncSSLHandshake(std::function<void(const boost::system::error_code& err)> handler);
	
	bool IsSSL() const { return m_socket.IsSSL(); }
	
	void OnDone(NewsClientCommand& cmd, const DoneCode code);

	void SetTimeout();
	void CancelTimeout();
	void CloseTimeout(const boost::system::error_code& error);
	void IncrementMissingSegmentCount() { m_server.Statistics.MissingSegmentCount++; }
	void IncrementConnectionTimeoutCount() { m_server.Statistics.ConnectionTimeoutCount++; }
	void IncrementErrorCount() { m_server.Statistics.ErrorCount++; }

	virtual void NoSuchGroupOnServer(const std::string& group){}
	virtual std::vector<std::string> FilterBlacklistedGroups(const std::vector<std::string>& groups) { return groups; }

};

std::ostream& operator<<(std::ostream& ost, const NewsClient::DoneCode code);



}

#endif
