/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef NEWSCLIENTSTATARTICLE_HPP
#define NEWSCLIENTSTATARTICLE_HPP

#include <memory>
#include <string>
#include "NewsClient.hpp"
#include "NewsClientCommand.hpp"
#include "Logger.hpp"
#include <boost/asio.hpp>
#include <functional>

namespace ByteFountain
{
	
typedef boost::asio::io_service io_service;

class NewsClientStatArticle : public NewsClientCommand
{
	typedef std::function<void (const bool success)> StatHandler;
	std::string m_group;
	std::string m_msgID;
	std::string m_lineBuffer;
	StatHandler m_handler;
	bool m_succeeded;

	void StatArticle(NewsClient& client);
	void HandleGROUP(NewsClient& client, const error_code& err, const std::size_t len);
	void HandleGROUPResponse(NewsClient& client, const error_code& err, const std::size_t len);
	void HandleSTAT(NewsClient& client, const error_code& err, const std::size_t len);
	void HandleSTATResponse(NewsClient& client, const error_code& err, const std::size_t len);
	
public:
	
	NewsClientStatArticle(io_service& ios, Logger& logger, const std::string& group, const std::string& msgID, const StatHandler& handler);
	virtual ~NewsClientStatArticle();
	virtual void Run(NewsClient& client);
	virtual std::ostream& print(std::ostream& ost) const;

};

}

#endif