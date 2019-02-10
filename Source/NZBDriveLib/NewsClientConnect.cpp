/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#include "NewsClientConnect.hpp"

namespace ByteFountain
{

NewsClientConnect::NewsClientConnect(io_service& ios, Logger& logger):
	NewsClientCommand(ios,logger),m_resolver(ios)
{}

/*virtual*/ NewsClientConnect::~NewsClientConnect(){}
/*virtual*/ void NewsClientConnect::Run(NewsClient& client){ StartConnect(client); }
/*virtual*/ std::ostream& NewsClientConnect::print(std::ostream& ost) const 
{
	ost<<"Usenet Login Command";
	return ost;
}

void NewsClientConnect::ErrorConnecting(NewsClient& client, const error_code& err)
{
	const NewsServer& server(client.GetServer());

	if (m_canceled)
	{
		m_logger << Logger::Debug << "Connecting to " << server << " failed with " << err.message() << Logger::End;
	}
	else
	{
		m_logger << Logger::Error << "Connecting to " << server << " failed with " << err.message() << Logger::End;
	}

	return ClientDisconnect(client);
}

void NewsClientConnect::SuccessConnecting(NewsClient& client)
{
	const NewsServer& server(client.GetServer());

	m_logger << Logger::Info << "Connected to "<<server<<Logger::End;
	return ClientDone(client);
}


void NewsClientConnect::StartConnect(NewsClient& client)
{
	const NewsServer& server(client.GetServer());

	m_logger<<Logger::Debug<<"Connecting to "<<server<<Logger::End;
	tcp_resolver::query query(server.Servername, server.Port);
	
	client.AsyncResolve(*this, m_resolver, query,
		[&client,this](const error_code& err, tcp_resolver::iterator endpoint_iterator)
		{
			HandleResolve(client,err, endpoint_iterator);
		}
	);
}
void NewsClientConnect::HandleResolve(NewsClient& client, const error_code& err, tcp_resolver::iterator endpoint_iterator)
{
	if (!err)
	{
		// Attempt a connection to the first endpoint in the list. Each endpoint
		// will be tried until we successfully establish a connection.
		tcp_endpoint endpoint = *endpoint_iterator;

		client.AsyncConnect(*this, endpoint,
			[&client, this, endpoint_iterator](const error_code& err) mutable
			{
				HandleConnect(client, err, ++endpoint_iterator);
			}
		);
		
	}
	else
	{
		return ErrorConnecting(client,err);
	}
}



void NewsClientConnect::HandleConnect(NewsClient& client, const error_code& err, tcp_resolver::iterator endpoint_iterator)
{
	if (!err && client.IsConnected())
	{
		if (client.IsSSL())
		{
			client.AsyncSSLHandshake(
				[&client, this](const error_code& err)
				{
					HandleAsyncHandshake(client, err);
				}
			);
		}
		else
		{
			client.AsyncReadResponse(*this,
				[&client, this](const error_code& err, const std::size_t len)
				{
					HandleReadConnectLine(client, err, len);
				}
			);
		}
	}
	else if (endpoint_iterator != boost::asio::ip::tcp::resolver::iterator())
	{
		// The connection failed. Try the next endpoint in the list.

		tcp_endpoint endpoint = *endpoint_iterator;
		
		client.AsyncConnect(*this, endpoint,
			[&client, this, endpoint_iterator](const error_code& err) mutable
			{
				HandleConnect(client, err, ++endpoint_iterator);
			});
	}
	else
	{
		return ErrorConnecting(client,err);
	}
}
void NewsClientConnect::HandleAsyncHandshake(NewsClient& client, const error_code& err)
{
	if (!err)
	{
		client.AsyncReadResponse(*this,
			[&client, this](const error_code& err, const std::size_t len) mutable
			{
				HandleReadConnectLine(client, err, len);
			}
		);
	}
	else
	{
		return ErrorConnecting(client,err);
	}
}
void NewsClientConnect::HandleReadConnectLine(NewsClient& client, const error_code& err, const std::size_t len)
{
	if (!err)
	{
		std::string status_message;
		unsigned int status_code;
		
		client.ReadStatusResponse(status_code, status_message);
		
		if (status_code == 0) return ClientReconnectRetry(client);
		
		if (status_code != 200)
		{
			if (status_code == 400) return ClientReconnectRetry(client);
			return ClientError(client);
		}

		const NewsServer& server(client.GetServer());
		
		if (server.Username.size()==0 && server.Password.size()==0)
		{
			return SuccessConnecting(client);
		}
		
		// The connection was successful. Send the login request.
		std::ostream request_stream(&m_request);
		request_stream << "AUTHINFO USER "<<server.Username<<"\r\n";
		request_stream << "AUTHINFO PASS "<<server.Password<<"\r\n";
		
		client.AsyncWriteRequest(*this,
			[&client, this](const error_code& err, const std::size_t len) mutable
			{
				HandleLoginRequest(client, err);
			}
		);
	}
	else
	{
		return ErrorConnecting(client,err);
	}
}

void NewsClientConnect::HandleLoginRequest(NewsClient& client, const error_code& err)
{
	if (!err)
	{
		client.AsyncReadResponse(*this,
			[&client, this](const error_code& err, const std::size_t len) mutable
			{
				HandleReadUserLine(client, err, len);
			}
		);
	}
	else
	{
		return ErrorConnecting(client,err);
	}
}

void NewsClientConnect::HandleReadUserLine(NewsClient& client, const error_code& err, const std::size_t len)
{
	if (!err)
	{
		std::string status_message;
		unsigned int status_code;
		
		client.ReadStatusResponse(status_code, status_message);

		if (status_code != 381)
		{
			if (status_code == 0 || status_code == 400)
			{
				m_logger << Logger::Warning << "Login user returned with status: "
					<< status_code << " " << status_message << Logger::End;

				return ClientReconnectRetry(client);
			}

			m_logger << Logger::Error << "Login to " << client.GetServer() << " failed with status: "
				<< status_code << " " << status_message << Logger::End;

			return ClientDisconnect(client);
		}

		client.AsyncReadResponse(*this,
			[&client, this](const error_code& err, const std::size_t len) mutable
			{
				HandleReadPassLine(client, err, len);
			}
		);
	}
	else
	{
		return ErrorConnecting(client,err);
	}
}

void NewsClientConnect::HandleReadPassLine(NewsClient& client, const error_code& err, const std::size_t len)
{
	if (!err)
	{
		std::string status_message;
		unsigned int status_code;
		
		client.ReadStatusResponse(status_code, status_message);
		
		if (status_code != 281)
		{
			if (status_code == 0 || status_code == 400)
			{
				m_logger << Logger::Warning << "Login user/pass returned with status: "
					<< status_code << " " << status_message << Logger::End;

				return ClientReconnectRetry(client);
			}

			m_logger << Logger::Error << "Login to " << client.GetServer() << " failed with status: "
				<< status_code << " " << status_message << Logger::End;

			return ClientDisconnect(client);
		}

		return SuccessConnecting(client);
	}
	else
	{
		return ErrorConnecting(client,err);
	}
}	

}
