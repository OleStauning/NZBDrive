/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#include "NewsClientReadOverview.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/asio/error.hpp>
#include <string.h>

namespace ByteFountain
{

void NewsClientReadOverview::ClientDone(NewsClient& client)
{
	m_handler->OnEnd(m_highID <= m_currentID);
	NewsClientCommand::ClientDone(client);
}
void NewsClientReadOverview::ClientCancel(NewsClient& client)
{
	m_handler->OnCancel();
	NewsClientCommand::ClientCancel(client);
}
void NewsClientReadOverview::ClientError(NewsClient& client)
{
	m_handler->OnErrorCancel();
	NewsClientCommand::ClientError(client);
}
void NewsClientReadOverview::ClientRetryOther(NewsClient& client)
{
	m_handler->OnErrorRetry();
	NewsClientCommand::ClientRetryOther(client);
}
void NewsClientReadOverview::ClientReconnectRetry(NewsClient& client)
{
	m_handler->OnErrorRetry();
	NewsClientCommand::ClientReconnectRetry(client);
}

NewsClientReadOverview::NewsClientReadOverview(io_service& ios, Logger& logger, const std::string& group,
	const unsigned long long fromID, const unsigned long long toID,
	std::shared_ptr<IOverviewResponseHandler> handler):
	NewsClientCommand(ios,logger), m_group(group), m_fromID(fromID), m_toID(toID),
	m_nextID(fromID), m_currentID(fromID), m_highID(toID), m_handler(handler)
{}

/*virtual*/ NewsClientReadOverview::~NewsClientReadOverview()
{
}

/*virtual*/ void NewsClientReadOverview::OnCommandCancel()
{
}

/*virtual*/ void NewsClientReadOverview::Run(NewsClient& client){ GetOverview(client); }



/*virtual*/ std::ostream& NewsClientReadOverview::print(std::ostream& ost) const
{
	ost<<"XOVER "<<m_fromID<<" - "<<m_toID<<" in group '"<<m_group<<"'";
	return ost;
}

void NewsClientReadOverview::GetOverview(NewsClient& client)
{
	std::ostream request_stream(&m_request);
	request_stream << "GROUP "<<m_group<<"\r\n";

	client.AsyncWriteRequest(*this,
		[&client, this](const error_code& err, const std::size_t len) mutable
		{
			HandleRequestGROUP(client, err, len);
		}
	);
}


void NewsClientReadOverview::HandleRequestGROUP(NewsClient& client, const error_code& err, const std::size_t len)
{
	if (!err)
	{
		client.AsyncReadResponse(*this,
			[&client, this](const error_code& err, const std::size_t len) mutable
			{
				HandleRequestGROUPResponse(client, err, len);
			}
		);
	}
	else
	{
		HandleError(client,err,len);
	}
}

void NewsClientReadOverview::HandleRequestGROUPResponse(NewsClient& client, const error_code& err, const std::size_t len)
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
		if (status_code != 211)
		{
			if (status_code == 411) // No such group
			{
				m_logger << Logger::Warning << "GROUP response returned with status: "
					<< status_code << " " << status_message << Logger::End;
				m_handler->OnGroupResponse(false,0,0,0);
				return ClientRetryOther(client);
			}
			else if (status_code == 400 
				|| status_code == 503 /*time out*/ 
				|| status_code == 500 /*Max connect time exceeded */)
			{
				m_logger << Logger::Warning << "GROUP response returned with status: "
					<< status_code << " " << status_message << Logger::End;
				return ClientReconnectRetry(client);
			}

			m_logger << Logger::Error << "GROUP response returned with status: "
				<< status_code << " " << status_message << Logger::End;
			return ClientError(client);
		}
		
		
		boost::trim_right_if(status_message,boost::is_any_of("\r\n"));

		std::vector<std::string> strs;
		boost::split(strs,status_message,boost::is_any_of(" \t"));

		unsigned long long size;
		unsigned long long low;
		unsigned long long high;
		
		try
		{
			size = std::stoull(strs[0]);
			low = std::stoull(strs[1]);
			high = std::stoull(strs[2]);
		}
		catch(const std::exception& e)
		{
			m_logger << Logger::Error <<"Could not read (size,low,high) from: "<< 
				strs[0] << " " << strs[1] << " " << strs[2] << Logger::End;
			throw e;
		}
		
		m_logger << Logger::Debug<<"size "<<size<<Logger::End;
		m_logger << Logger::Debug<<"low "<<low<<Logger::End;
		m_logger << Logger::Debug<<"high "<<high<<Logger::End;

		m_handler->OnGroupResponse(true,size,low,high);
				
		if (m_fromID>m_toID) return ClientDone(client);

		m_highID = high;

		if (m_fromID >= high)
		{
			m_logger << Logger::Info<<"No new messages: "<<status_message<<Logger::End;
			return ClientDone(client);
		}
		
		m_fromID=std::max(m_fromID,low);
		m_toID=std::min(m_toID,high);
		
		m_nextID=std::min(m_toID,m_fromID+1000000); // XOVER limited number.
		
		m_currentID=m_fromID;
		
		std::ostream request_stream(&m_request);
		request_stream << "XOVER "<<m_fromID<<"-"<<m_nextID<<"\r\n";

		client.AsyncWriteRequest(*this,
			[&client, this](const error_code& err, const std::size_t len) mutable
			{
				HandleRequestXOVER(client, err, len);
			}
		);
		
	}
	else
	{
		HandleError(client,err,len);
	}
}


void NewsClientReadOverview::HandleRequestXOVER(NewsClient& client, const error_code& err, const std::size_t len)
{
	if (!err)
	{
		client.AsyncReadResponse(*this,
			[&client, this](const error_code& err, const std::size_t len) mutable
			{
				HandleRequestXOVERResponse(client, err, len);
			}
		);
	}
	else
	{
		HandleError(client,err,len);
	}
}

void NewsClientReadOverview::HandleRequestXOVERResponse(NewsClient& client, const error_code& err, const std::size_t len)
{
	if (!err)
	{
		std::string status_message;
		unsigned int status_code;

		m_logger << Logger::Debug<<status_message<<Logger::End;
		
		
		if (!client.ReadStatusResponse(status_code, status_message))
		{
			m_logger << Logger::Warning<<"Invalid response"<<Logger::End;
			return ClientReconnectRetry(client);
		}
				
		client.AsyncReadDataResponse(*this,
			[&client, this](const error_code& err, const std::size_t len) mutable
			{
				HandleReadXOVERStream(client, err, len);
			}
		);
	}
	else
	{
		HandleError(client,err,len);
	}
}

void NewsClientReadOverview::HandleReadXOVERStream(NewsClient& client, const error_code& err, const std::size_t size)
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
				
				if (m_nextID<m_toID)
				{
					m_fromID=m_nextID+1;
					return GetOverview(client);
				}
				
				return ClientDone(client);
			}
			else
			{
				std::string value(msgBegin, size);
				boost::trim_right_if(value,boost::is_any_of("\r\n"));
				std::vector<std::string> strs;
				boost::split(strs,value,boost::is_any_of("\t"));
				
				try
				{
				
					m_currentID = std::stoull(strs[0]);
				}
				catch(const std::exception& e)
				{
					m_logger << Logger::Error <<"Could not read article ID from: "<< 
						strs[0] << Logger::End;
					throw e;
				}
				
				m_handler->OnOverviewResponse(
					OverviewResponse(
						value,
						strs[0],
						strs[1],
						strs[2],
						strs[3],
						strs[4],
						strs[6],
						strs[7],
						strs[8]
					)
				);
			}
		}

		client.Consume(size);
		
		client.AsyncReadDataResponse(*this,
			[&client, this](const error_code& err, const std::size_t len) mutable
			{
				HandleReadXOVERStream(client, err, len);
			}
		);
	}
	else
	{
		HandleError(client,err,size-2);
	}
}

}
