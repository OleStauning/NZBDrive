/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef NEWSCLIENTCONNECT_HPP
#define NEWSCLIENTCONNECT_HPP

#include "Logger.hpp"
#include "NewsClient.hpp"
#include "NewsClientCommand.hpp"
#include <boost/asio.hpp>

namespace ByteFountain
{

typedef boost::asio::ip::tcp::resolver tcp_resolver;
typedef boost::asio::io_service io_service;


class NewsClientConnect : public NewsClientCommand
{
	tcp_resolver m_resolver;

	void ErrorConnecting(NewsClient& client, const error_code& err);
	void SuccessConnecting(NewsClient& client);
	void StartConnect(NewsClient& client);
	void HandleResolve(NewsClient& client, const error_code& err, tcp_resolver::iterator endpoint_iterator);
	void HandleConnect(NewsClient& client, const error_code& err, tcp_resolver::iterator endpoint_iterator);
	void HandleAsyncHandshake(NewsClient& client, const error_code& err);
	void HandleReadConnectLine(NewsClient& client, const error_code& err, const std::size_t len);
	void HandleLoginRequest(NewsClient& client, const error_code& err);
	void HandleReadUserLine(NewsClient& client, const error_code& err, const std::size_t len);
	void HandleReadPassLine(NewsClient& client, const error_code& err, const std::size_t len);

public:
	NewsClientConnect(io_service& ios, Logger& logger);
	virtual ~NewsClientConnect();
	virtual void Run(NewsClient& client);
	void OnCommandCancel() 
	{
		m_resolver.cancel(); NewsClientCommand::OnCommandCancel();
	}
	virtual std::ostream& print(std::ostream& ost) const;
};

}

#endif

