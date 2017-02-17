/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#include "NewsClientCommand.hpp"
#include <boost/algorithm/string.hpp>

namespace ByteFountain
{
	
NewsClientCommand::NewsClientCommand(io_service& ios, Logger& logger):
m_logger(logger)/*, m_timer(ios)*/, m_request(ASIOMaxBufferSize), m_canceled(false)
{
}

/*virtual*/ NewsClientCommand::~NewsClientCommand()
{
//	m_timer.cancel();
}
	
void NewsClientCommand::ClientDone(NewsClient& client)
{
//	m_timer.cancel();
	m_logger << Logger::Debug << "Command finished" << Logger::End;
	client.OnDone(*this,NewsClient::OK);
}
void NewsClientCommand::ClientCancel(NewsClient& client)
{
//	m_timer.cancel();
	m_logger << Logger::Debug << "Command canceled" << Logger::End;
	client.OnDone(*this,NewsClient::OK);
}
void NewsClientCommand::ClientError(NewsClient& client)
{
//	m_timer.cancel();
	m_logger << Logger::Debug << "Command failed" << Logger::End;
	client.OnDone(*this,NewsClient::Error);
}
void NewsClientCommand::ClientDisconnect(NewsClient& client)
{
//	m_timer.cancel();
	client.OnDone(*this,NewsClient::DisconnectClient);
}
void NewsClientCommand::ClientRetryOther(NewsClient& client)
{
//	m_timer.cancel();
	m_logger << Logger::Debug << "Command failed, retry other server" << Logger::End;
	client.OnDone(*this,NewsClient::RetryOther);
}
void NewsClientCommand::ClientReconnectRetry(NewsClient& client)
{
//	m_timer.cancel();
	m_logger << Logger::Debug << "Command failed, reconnect and retry" << Logger::End;
	client.OnDone(*this,NewsClient::ReconnectRetry);
}

void NewsClientCommand::HandleError(NewsClient& client, const error_code& err, const std::size_t len)
{

	if (client.TimeoutOccurred())
	{
		client.IncrementConnectionTimeoutCount();

		return ClientReconnectRetry(client);
	}
	if (client.Disconnecting()) return ClientDisconnect(client);

#ifdef WIN32
	if (err.value()==WSAECONNABORTED || err.value()==WSAECONNRESET || err.value()==WSAECANCELLED || 
		err.value()==boost::asio::error::misc_errors::eof || err.value()==boost::asio::error::basic_errors::broken_pipe)
#else
	if (err.value()==ECONNRESET || err.value()==ECANCELED || err.value()==boost::asio::error::misc_errors::eof || err.value()==boost::asio::error::basic_errors::broken_pipe)
#endif
	{
		m_logger << Logger::Warning<<err.category().name()<<"  : "<<err.message()<<" ("<<err<<")"<< Logger::End;

		client.IncrementErrorCount();

		return ClientReconnectRetry(client);
	}

	m_logger << Logger::Error<<err.category().name()<<"  : "<<err.message()<<" ("<<err<<")"<< Logger::End;
	return ClientError(client);
}

void NewsClientCommand::OnCommandCancel()
{
	m_canceled = true;
}



}
