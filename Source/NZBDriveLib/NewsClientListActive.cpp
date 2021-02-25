/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#include "NewsClientListActive.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/asio/error.hpp>
#include <string.h>

namespace ByteFountain
{

void NewsClientListActive::ClientDone(NewsClient& client)
{
	NewsClientCommand::ClientDone(client);
}
void NewsClientListActive::ClientCancel(NewsClient& client)
{
	NewsClientCommand::ClientCancel(client);
}
void NewsClientListActive::ClientError(NewsClient& client)
{
	NewsClientCommand::ClientError(client);
}
void NewsClientListActive::ClientRetryOther(NewsClient& client)
{
	NewsClientCommand::ClientRetryOther(client);
}
void NewsClientListActive::ClientReconnectRetry(NewsClient& client)
{
	NewsClientCommand::ClientReconnectRetry(client);
}

NewsClientListActive::NewsClientListActive(
	io_service& ios, Logger& logger, 
	const std::string& wildmat, std::shared_ptr<IListActiveHandler> handler):
	NewsClientCommand(ios,logger), m_wildmat(wildmat), m_handler(handler)
{}

/*virtual*/ NewsClientListActive::~NewsClientListActive()
{
}

/*virtual*/ void NewsClientListActive::OnCommandCancel()
{
}

/*virtual*/ void NewsClientListActive::Run(NewsClient& client){ GetActiveGroups(client); }



/*virtual*/ std::ostream& NewsClientListActive::print(std::ostream& ost) const
{
	ost<<"LIST ACTIVE "<<m_wildmat;
	return ost;
}

void NewsClientListActive::GetActiveGroups(NewsClient& client)
{
	std::ostream request_stream(&m_request);
	request_stream << "LIST ACTIVE "<<m_wildmat<<"\r\n";

	client.AsyncWriteRequest(*this,
		[&client, this](const error_code& err, const std::size_t len) mutable
		{
			HandleListActiveRequest(client, err, len);
		}
	);
}


void NewsClientListActive::HandleListActiveRequest(NewsClient& client, const error_code& err, const std::size_t len)
{
	if (!err)
	{
		client.AsyncReadResponse(*this,
			[&client, this](const error_code& err, const std::size_t len) mutable
			{
				HandleListActiveResponse(client, err, len);
			}
		);
	}
	else
	{
		HandleError(client,err,len);
	}
}

void NewsClientListActive::HandleListActiveResponse(NewsClient& client, const error_code& err, const std::size_t len)
{
	if (!err)
	{
		std::string status_message;
		unsigned int status_code;

		
		if (!client.ReadStatusResponse(status_code, status_message))
		{
			m_logger << Logger::Warning<<"Invalid response"<<Logger::End;
			return ClientReconnectRetry(client);
		}
				
		m_logger << Logger::Debug<< "Status message: " <<status_message << Logger::End;
		
		client.AsyncReadDataResponse(*this,
			[&client, this](const error_code& err, const std::size_t len) mutable
			{
				HandleListActiveResponseStream(client, err, len);
			}
		);
	}
	else
	{
		HandleError(client,err,len);
	}
}

void NewsClientListActive::HandleListActiveResponseStream(NewsClient& client, const error_code& err, const std::size_t size)
{
//		m_timer.cancel();
		
	if (!err)
	{
		const char* msgBegin=client.Data();
		
		if (size>0)
		{
	
			if (*msgBegin=='.' && size==3)
			{
				client.Consume(3);
				
				return ClientDone(client);
			}
			else
			{
	
				std::string value(msgBegin, size);
				boost::trim_right_if(value,boost::is_any_of("\r\n"));
				std::vector<std::string> strs;
				boost::split(strs,value,boost::is_any_of(" \t"));

				m_handler->OnActiveGroupInfo(value, strs[0], strs[1], strs[2], strs[3]);
			}
		}

		client.Consume(size);
		
		client.AsyncReadDataResponse(*this,
			[&client, this](const error_code& err, const std::size_t len) mutable
			{
				HandleListActiveResponseStream(client, err, len);
			}
		);
	}
	else
	{
		m_logger << Logger::Error<< "HandleListActiveResponseStream: LIST ACTIVE Failed" << Logger::End;
		HandleError(client,err,size-2);
	}
}

}
