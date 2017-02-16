/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#include "NewsClientStatArticle.hpp"

namespace ByteFountain
{

NewsClientStatArticle::NewsClientStatArticle(io_service& ios, Logger& logger, const std::string& group, const std::string& msgID, const StatHandler& handler):
	NewsClientCommand(ios,logger), m_group(group), m_msgID(msgID), m_handler(handler), m_succeeded(false)
{}

/*virtual*/ NewsClientStatArticle::~NewsClientStatArticle()
{
	if (!m_succeeded) m_handler(false);
}

/*virtual*/ void NewsClientStatArticle::Run(NewsClient& client){ StatArticle(client); }
/*virtual*/ std::ostream& NewsClientStatArticle::print(std::ostream& ost) const
{
	ost<<"Stat '"<<m_msgID<<"' in group '"<<m_group<<"'";
	return ost;
}


void NewsClientStatArticle::StatArticle(NewsClient& client)
{
	std::ostream request_stream(&m_request);
	request_stream << "GROUP "<<m_group<<"\r\n";
	request_stream << "STAT <"<<m_msgID<<">\r\n";

	client.AsyncWriteRequest(*this,
		[&client, this](const error_code& err, const std::size_t len) mutable
		{
			HandleGROUP(client, err, len);
		}
	);
}

void NewsClientStatArticle::HandleGROUP(NewsClient& client, const error_code& err, const std::size_t len)
{
	if (!err)
	{
		client.AsyncReadResponse(*this,
			[&client, this](const error_code& err, const std::size_t len) mutable
			{
				HandleGROUPResponse(client, err, len);
			}
		);
	}
	else
	{
		HandleError(client,err,len);
	}
}

void NewsClientStatArticle::HandleGROUPResponse(NewsClient& client, const error_code& err, const std::size_t len)
{
	if (!err)
	{
		std::string status_message;
		unsigned int status_code;

		if (!client.ReadStatusResponse(status_code, status_message))
		{
			m_logger << Logger::Error<<"Invalid response"<<Logger::End;
			return ClientReconnectRetry(client);
		}
		if (status_code != 211)
		{
			if (status_code == 400)
			{
				m_logger << Logger::Warning<<"GROUP response returned with status: "
					<< status_code << " "<<m_lineBuffer<<Logger::End;
				return ClientReconnectRetry(client);
			}

			m_logger << Logger::Error<<"GROUP response returned with status: "
				<< status_code << " "<<m_lineBuffer<<Logger::End;

			return ClientError(client);
		}

		client.AsyncReadResponse(*this,
			[&client, this](const error_code& err, const std::size_t len) mutable
			{
				HandleSTATResponse(client, err, len);
			}
		);
	}
	else
	{
		HandleError(client,err,len);
	}
}


void NewsClientStatArticle::HandleSTAT(NewsClient& client, const error_code& err, const std::size_t len)
{
	if (!err)
	{
		client.AsyncReadResponse(*this,
			[&client, this](const error_code& err, const std::size_t len) mutable
			{
				HandleSTATResponse(client, err, len);
			}
		);
	}
	else
	{
		HandleError(client,err,len);
	}
}

void NewsClientStatArticle::HandleSTATResponse(NewsClient& client, const error_code& err, const std::size_t len)
{
	if (!err)
	{
		std::string status_message;
		unsigned int status_code;

		if (!client.ReadStatusResponse(status_code, status_message))
		{
			m_logger << Logger::Error<<"Invalid response"<<Logger::End;
			return ClientReconnectRetry(client);
		}
		if (status_code != 223)
		{
			if (status_code == 400)
			{
				m_logger << Logger::Warning<<"Stat response returned with status: "
					<< status_code << " "<<status_message<<Logger::End;
				return ClientReconnectRetry(client);
			}

			m_logger << Logger::Error<<"Stat response returned with status: "
				<< status_code << " "<<status_message<<Logger::End;

			return ClientError(client);
		}
		
		m_succeeded=true;
		m_handler(true);
		
	}
	else
	{
		HandleError(client,err,len);
	}
}
	
	
	
}
