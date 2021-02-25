/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef NEWSCLIENTLISTACTIVE_HPP
#define NEWSCLIENTLISTACTIVE_HPP

#include <memory>
#include <string>
#include "NewsClient.hpp"
#include "NewsClientCommand.hpp"
#include "IListActiveHandler.hpp"
#include "Logger.hpp"
#include <boost/asio.hpp>


namespace ByteFountain
{
	
typedef boost::asio::io_service io_service;

class NewsClientListActive : public NewsClientCommand
{
	std::shared_ptr<IListActiveHandler> m_handler;
	std::string m_wildmat;
	
	void ClientDone(NewsClient& client);
	void ClientCancel(NewsClient& client);
	void ClientError(NewsClient& client);
	void ClientRetryOther(NewsClient& client);
	void ClientReconnectRetry(NewsClient& client);

	void GetActiveGroups(NewsClient& client);
	void HandleListActiveRequest(NewsClient& client, const error_code& err, const std::size_t len);
	void HandleListActiveResponse(NewsClient& client, const error_code& err, const std::size_t len);
	void HandleListActiveResponseStream(NewsClient& client, const error_code& err, const std::size_t size);
	
public:
	
	NewsClientListActive(io_service& ios, Logger& logger, const std::string& wildmat,
		std::shared_ptr<IListActiveHandler> handler);
	virtual ~NewsClientListActive();
	virtual void Run(NewsClient& client);
	virtual std::ostream& print(std::ostream& ost) const;

	virtual bool Pipeable() const { return false; }
	
	void OnCommandCancel();
};

}

#endif
