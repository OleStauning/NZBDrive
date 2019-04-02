/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#include "NewsClientReadArticleStream.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/asio/error.hpp>
#include <string.h>

namespace ByteFountain
{

void NewsClientReadArticleStream::ClientDone(NewsClient& client)
{
	m_handler->OnEnd();
	NewsClientCommand::ClientDone(client);
}
void NewsClientReadArticleStream::ClientCancel(NewsClient& client)
{
	m_handler->OnCancel();
	NewsClientCommand::ClientCancel(client);
}
void NewsClientReadArticleStream::ClientError(NewsClient& client)
{
	m_handler->OnErrorCancel();
	NewsClientCommand::ClientError(client);
}
void NewsClientReadArticleStream::ClientRetryOther(NewsClient& client)
{
	m_handler->OnErrorRetry();
	NewsClientCommand::ClientRetryOther(client);
}
void NewsClientReadArticleStream::ClientReconnectRetry(NewsClient& client)
{
	m_handler->OnErrorRetry();
	NewsClientCommand::ClientReconnectRetry(client);
}

	
NewsClientReadArticleStream::NewsClientReadArticleStream(io_service& ios, Logger& logger, 
	const std::vector<std::string>& groups, const std::vector<std::string>& msgIDs, 
	std::shared_ptr<IArticleStreamHandler> handler):
	NewsClientCommand(ios,logger), m_handler(handler), m_groups_init(groups), m_msgIDs_init(msgIDs)
{}

/*virtual*/ NewsClientReadArticleStream::~NewsClientReadArticleStream()
{
}

/*virtual*/ void NewsClientReadArticleStream::OnCommandCancel()
{
}

/*virtual*/ void NewsClientReadArticleStream::Run(NewsClient& client)
{ 
	m_groups = client.FilterBlacklistedGroups(m_groups_init);
	
	if (m_groups.empty()) return ClientRetryOther(client);
	
	m_msgIDs = m_msgIDs_init;
	
	GetArticleStream(client); 
}



/*virtual*/ std::ostream& NewsClientReadArticleStream::print(std::ostream& ost) const
{
	
	ost<<"Read ( ";
	copy(m_msgIDs_init.begin(),m_msgIDs_init.end(), std::ostream_iterator<std::string>(ost," "));	
	ost<<") in group ( ";
	copy(m_groups_init.begin(),m_groups_init.end(), std::ostream_iterator<std::string>(ost," "));	
	ost<<")";
	return ost;
}

void NewsClientReadArticleStream::GetArticleStream(NewsClient& client)
{
	auto group = m_groups.back();
	auto msgid = "<"+m_msgIDs.back()+">";
	
	std::ostream request_stream(&m_request);
	request_stream << "BODY "<<msgid<<"\r\n";

	client.AsyncWriteRequest(*this,
		[&client, this, msgid](const error_code& err, const std::size_t len) mutable
		{
			HandleBodyResponse(client, err, len, msgid);
		}
	);
}


void NewsClientReadArticleStream::HandleBodyResponse(NewsClient& client, const error_code& err, const std::size_t len, const std::string& msgid)
{
	if (!err)
	{
		std::string status_message_GROUP;
		unsigned int status_code_GROUP;

		client.AsyncReadResponse(*this, 
			[&client, this, status_message_GROUP, status_code_GROUP, msgid]
			(const error_code& err, const std::size_t len) mutable
				{
					HandleBodyRequestBODYResponse(client, status_message_GROUP, status_code_GROUP, err, len, msgid);
				}
		);
		
	}
	else
	{
		HandleError(client,err,len);
	}
}


void NewsClientReadArticleStream::HandleBodyRequestBODYResponse(NewsClient& client, 
	const std::string& status_message_GROUP, const unsigned int status_code_GROUP, 
	const error_code& err, const std::size_t len, const std::string& msgid)
{
	if (!err)
	{
		std::string status_message_BODY;
		unsigned int status_code_BODY;

		if (!client.ReadStatusResponse(status_code_BODY, status_message_BODY))
		{
			m_logger << Logger::Warning<<"Invalid response"<<Logger::End;
			return ClientReconnectRetry(client);
		}
		if (status_code_BODY != 222)
		{
			if (status_code_BODY == 430) // "no such article" or "DMCA Removed"
			{
				m_logger<<Logger::Warning<<"Article "<<msgid<<" response returned with status: "
					<< status_code_BODY << " " << status_message_BODY << Logger::End;

				client.IncrementMissingSegmentCount();
				
				m_msgIDs.pop_back();
/*
 * BROKEN PIPELINE:
				if (!m_msgIDs.empty())
				{
					m_logger << Logger::Warning << "Retrying with redundant message" << Logger::End;
					return GetArticleStream(client);
				}
*/					
				return ClientRetryOther(client);
			}
			else if (status_code_BODY == 400 
				|| status_code_BODY == 503 /*time out*/ 
				|| status_code_BODY == 500 /*Max connect time exceeded */)
			{
				m_logger<<Logger::Warning<<"Article "<<msgid<<" response returned with status: "
					<< status_code_BODY << " " << status_message_BODY << Logger::End;

				client.IncrementConnectionTimeoutCount();

				return ClientReconnectRetry(client);
			}
			m_logger<<Logger::Error<<"Article "<<msgid<<" response returned with status: "
				<< status_code_BODY << " " << status_message_BODY << Logger::End;
			return ClientReconnectRetry(client);
		}
		
		if (!boost::algorithm::ends_with(status_message_BODY, msgid))
		{
			m_logger<< Logger::Error << "RESPONSE MSG: " << status_message_BODY 
				<< "REQUESTED: " << msgid << Logger::End;
			
			return ClientReconnectRetry(client);
		}

		client.AsyncReadDataResponse(*this,
			[&client, this](const error_code& err, const std::size_t len) mutable
			{
				HandleReadBodyStream(client, err, len);
			}
		);
	}
	else
	{
		HandleError(client,err,len);
	}
}

void NewsClientReadArticleStream::HandleReadBodyStream(NewsClient& client, const error_code& err, const std::size_t size)
{
	if (!err)
	{
		const char* msgBegin=client.Data();
		
		if (size>0)
		{
			if (*msgBegin=='.')
			{
				if (size==3)
				{
//					m_timer.cancel();
					client.Consume(3);
					return ClientDone(client);
				}
				m_handler->OnData(msgBegin+1,size-3);
			}
			else
			{
				m_handler->OnData(msgBegin,size-2);
			}
		}

		client.Consume(size);
		
		client.AsyncReadDataResponse(*this,
			[&client, this](const error_code& err, const std::size_t len) mutable
			{
				HandleReadBodyStream(client, err, len);
			}
		);
	}
	else
	{
		HandleError(client,err,size-2);
	}
}

}
