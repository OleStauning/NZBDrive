/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef NEWSCLIENTREADARTICLESTREAM_HPP
#define NEWSCLIENTREADARTICLESTREAM_HPP

#include <memory>
#include <string>
#include "NewsClient.hpp"
#include "NewsClientCommand.hpp"
#include "IArticleStreamHandler.hpp"
#include "Logger.hpp"
#include <boost/asio.hpp>


namespace ByteFountain
{
	
typedef boost::asio::io_service io_service;

class NewsClientReadArticleStream : public NewsClientCommand
{
	std::shared_ptr<IArticleStreamHandler> m_handler;

	std::vector<std::string> m_groups_init;
	std::vector<std::string> m_msgIDs_init;
	
	std::vector<std::string> m_groups;
	std::vector<std::string> m_msgIDs;

	std::string m_lineBuffer;

	void ClientDone(NewsClient& client);
	void ClientCancel(NewsClient& client);
	void ClientError(NewsClient& client);
	void ClientRetryOther(NewsClient& client);
	void ClientReconnectRetry(NewsClient& client);


	void GetArticleStream(NewsClient& client);
	void HandleBodyRequestGROUP(NewsClient& client, const error_code& err, const std::size_t len, const std::string& msgid);
	void HandleBodyRequestGROUPResponse(NewsClient& client, const error_code& err, const std::size_t len, const std::string& msgid);
	void HandleBodyRequestBODY(NewsClient& client, const error_code& err, const std::size_t len);
	void HandleBodyRequestBODYResponse(NewsClient& client, 
		const std::string& status_message_GROUP, const unsigned int status_code_GROUP,
		const error_code& err, const std::size_t len, const std::string& msgid);
	void HandleReadBodyStream(NewsClient& client, const error_code& err, const std::size_t len);
	
public:
	
	NewsClientReadArticleStream(io_service& ios, Logger& logger, 
		const std::vector<std::string>& groups, const std::vector<std::string>& msgIDs, 
		std::shared_ptr<IArticleStreamHandler> handler);
	virtual ~NewsClientReadArticleStream();
	virtual void Run(NewsClient& client);
	virtual std::ostream& print(std::ostream& ost) const;

	virtual bool Pipeable() const { return true; }
	
	void OnCommandCancel();
};

}

#endif
