/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef NEWSCLIENTREADOVERVIEW_HPP
#define NEWSCLIENTREADOVERVIEW_HPP

#include <memory>
#include <string>
#include "NewsClient.hpp"
#include "NewsClientCommand.hpp"
#include "IOverviewResponseHandler.hpp"
#include "Logger.hpp"
#include <boost/asio.hpp>


namespace ByteFountain
{
	
typedef boost::asio::io_service io_service;

class NewsClientReadOverview : public NewsClientCommand
{
	const std::string m_group;
	unsigned long long m_fromID;
	unsigned long long m_toID;
	unsigned long long m_nextID;
	unsigned long long m_currentID;
	unsigned long long m_highID;
	std::shared_ptr<IOverviewResponseHandler> m_handler;
	
	void ClientDone(NewsClient& client);
	void ClientCancel(NewsClient& client);
	void ClientError(NewsClient& client);
	void ClientRetryOther(NewsClient& client);
	void ClientReconnectRetry(NewsClient& client);

	void GetOverview(NewsClient& client);
	void HandleRequestGROUP(NewsClient& client, const error_code& err, const std::size_t len);
	void HandleRequestGROUPResponse(NewsClient& client, const error_code& err, const std::size_t len);
	void HandleRequestXOVER(NewsClient& client, const error_code& err, const std::size_t len);
	void HandleRequestXOVERResponse(NewsClient& client, const error_code& err, const std::size_t len);
	void HandleReadXOVERStream(NewsClient& client, const error_code& err, const std::size_t len);
	
public:
	
	NewsClientReadOverview(io_service& ios, Logger& logger, const std::string& group,
		const unsigned long long fromID,
		const unsigned long long toID,
		std::shared_ptr<IOverviewResponseHandler> handler);
	virtual ~NewsClientReadOverview();
	virtual void Run(NewsClient& client);
	virtual std::ostream& print(std::ostream& ost) const;

	virtual bool Pipeable() const { return false; }
	
	void OnCommandCancel();
};

}

#endif