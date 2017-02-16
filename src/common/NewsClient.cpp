/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#include "NewsServer.hpp"
#include "NewsClient.hpp"
#include "NewsClientCommand.hpp"
#include <boost/algorithm/string.hpp>

namespace ByteFountain
{

	void NewsClient::Disconnect()
	{
		m_disconnecting = true;
		if (m_active)
		{
			m_active->OnCommandCancel();
		}
		m_socket.close();
		if (m_handles == 0) OnStopped();
	}


	bool NewsClient::ReadStatusResponse(unsigned int& status_code, std::string& message)
	{
		std::istream response_stream(&m_response);
		response_stream >> status_code;
		char space;
		response_stream.get(space);
		std::getline(response_stream, message);

		if (!response_stream)
		{
			m_logger << Logger::Error << "Invalid response" << Logger::End;
			status_code = 0;
		}
		return response_stream.good();
	}

	void NewsClient::AsyncWriteRequest(NewsClientCommand& cmd, const std::function<void(const error_code& err, const std::size_t len)>& func)
	{
		std::ostringstream oss;

		std::copy(boost::asio::buffer_cast<const char *>(cmd.RequestBuffer().data()), 
			boost::asio::buffer_cast<const char *>(cmd.RequestBuffer().data()) + cmd.RequestBuffer().size(), std::ostream_iterator<char>(oss)); 
		
		std::string res=oss.str();
		boost::trim(res);
		
		m_logger<<Logger::Debug<<"Sending: "<<res<<Logger::End;

		SetTimeout();

		HandleCount count(*this);
		boost::asio::async_write(m_socket, cmd.RequestBuffer(),
			[this, func, count](const error_code& err, const std::size_t len)
			{
				m_server.Statistics.BytesTX += len;
				CancelTimeout();
				func(err,len);
			});
		
	}
	
	void NewsClient::AsyncReadResponse(NewsClientCommand& cmd, const std::function<void(const error_code& err, const std::size_t len)>& func)
	{
		if (m_active != 0 && m_active != &cmd)
		{
			m_pipeline.emplace( PipeCmd(&cmd,func) );
		}
		else
		{
			m_active=&cmd;
		
			SetTimeout();
			HandleCount count(*this);
			boost::asio::async_read_until(m_socket, m_response, "\r\n",
				[this, &cmd, func, count](const error_code& err, const std::size_t len)
				{
					m_server.Statistics.BytesRX += len;
					CancelTimeout();
					if (!err)
					{
						std::ostringstream oss;
						
						std::copy(boost::asio::buffer_cast<const char *>(m_response.data()), 
							boost::asio::buffer_cast<const char *>(m_response.data()) +len, std::ostream_iterator<char>(oss)); 
						
						std::string res=oss.str();
						boost::trim(res);
						
						m_logger<<Logger::Debug<<"Receiving: "<<res<<Logger::End;
					}
					func(err,len);
				}
			);
			
		}
	}
	
	void NewsClient::AsyncReadDataResponse(NewsClientCommand& cmd, const std::function<void(const error_code& err, const std::size_t len)>& func)
	{
		SetTimeout();

		HandleCount count(*this);
		boost::asio::async_read_until(m_socket, m_response, "\r\n",
			[this, func, count](const error_code& err, const std::size_t len)
			{
				m_server.Statistics.BytesRX += len;
				CancelTimeout();
				func(err,len);
			}
		);
		
	}

	void NewsClient::AsyncResolve(NewsClientCommand& cmd, tcp_resolver& resolver, const tcp_resolver::query& query,
		std::function<void(const error_code& err, tcp_resolver::iterator endpoint_iterator)> handler)
	{
		HandleCount count(*this);
	
		m_active = &cmd; // Ffor cancelation of resolve....

		resolver.async_resolve(query, 
			[this, handler, count](const error_code& err, tcp_resolver::iterator endpoint_iterator)
			{
				m_active = 0;
				handler(err,endpoint_iterator);
			}
		);
	}

	void NewsClient::AsyncConnect(NewsClientCommand& cmd, const tcp_endpoint& endpoint, std::function<void(const error_code& err)> handler)
	{
		HandleCount count(*this);

		m_active = &cmd; // Ffor cancelation of resolve....
		
		if (m_socket.is_open()) m_socket.close();
		
		m_socket.async_connect(endpoint,
			[this, handler, count](const error_code& err)
			{
				m_active = 0;
				handler(err);
			}
		);
	}

	void NewsClient::AsyncSSLHandshake(std::function<void(const error_code& err)> handler)
	{
		HandleCount count(*this);

		m_socket.async_ssl_handshake(
			[handler, count](const error_code& err)
			{
				handler(err);
			}
		);
	}
	
	void NewsClient::OnDone(NewsClientCommand& cmd, const DoneCode code)
	{ 
		CancelTimeout();

		m_active=0;

		m_done(*this, cmd, code);

		if (code == ReconnectRetry || code == DisconnectClient)
		{
			while (!m_pipeline.empty())
			{
				m_done(*this, *(m_pipeline.front().first), code);
				m_pipeline.pop();
			}
		}
		else if (!m_pipeline.empty())
		{
			PipeCmd pcmd=m_pipeline.front();
			m_pipeline.pop();
			AsyncReadResponse(*pcmd.first,pcmd.second);
		}

	}

	void NewsClient::OnStarted()
	{
		m_logger << Logger::Debug << "NewsClient Started" << Logger::End;
	}
	void NewsClient::OnStopped()
	{
		m_logger << Logger::Debug << "NewsClient Stopped" << Logger::End;
	}

	void NewsClient::SetTimeout()
	{
		m_timer.cancel();
		m_timer.expires_from_now(seconds(m_server.Timeout));
		HandleCount count(*this);
		m_timer.async_wait(
			[this, count](const boost::system::error_code& err)
			{ 
				if (m_disconnecting) return;
				if (!err && m_timer.expires_at() <= boost::asio::deadline_timer::traits_type::now())
				{
					CloseTimeout(err);
				}
			});
	}
	
	void NewsClient::CancelTimeout()
	{
		m_timer.cancel();
	}

	void NewsClient::CloseTimeout(const boost::system::error_code& error)
	{
		if (!error && !m_timeout)
		{
			m_logger << Logger::Warning<<"Timeout occurred, closing socket" << Logger::End;
			m_timeout=true;
			Disconnect();
		}
	}

	std::ostream& operator<<(std::ostream& ost, const NewsClient::DoneCode code)
	{
		switch(code)
		{
			case NewsClient::OK: ost<<"OK"; break;
			case NewsClient::Error: ost<<"Error"; break;
			case NewsClient::RetryOther: ost<<"RetryOther"; break;
			case NewsClient::ReconnectRetry: ost<<"ReconnectRetry"; break;
			case NewsClient::DisconnectClient: ost<<"DisconnectClient"; break;
		}
		return ost;
		
	}

}






