/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef NEWSCLIENTCOMMAND_HPP
#define NEWSCLIENTCOMMAND_HPP

#include <boost/asio.hpp>
#include "Logger.hpp"
#include "NewsClient.hpp"


namespace ByteFountain
{

typedef boost::asio::io_service io_service;
using namespace boost::system;

class NewsClientCommand 
{
protected:
	Logger& m_logger;
	
	unsigned long long m_rxBytes;
	boost::asio::streambuf m_request;

	void HandleError(NewsClient& client, const boost::system::error_code& err, const std::size_t len);
	
	bool m_canceled;

public:
	NewsClientCommand(io_service& ios, Logger& logger);
	virtual ~NewsClientCommand();
	virtual void Run(NewsClient& client) =0;
	virtual std::ostream& print(std::ostream&) const =0;
	
	boost::asio::streambuf& RequestBuffer() { return m_request; }
	
	virtual void ClientDone(NewsClient& client);
	virtual void ClientCancel(NewsClient& client);
	virtual void ClientError(NewsClient& client);
	virtual void ClientDisconnect(NewsClient& client);
	virtual void ClientRetryOther(NewsClient& client);
	virtual void ClientReconnectRetry(NewsClient& client);

	virtual void OnCommandCancel();

	unsigned long long RXBytes() const { return m_rxBytes; }
	
	virtual bool Pipeable() const { return false; }
};

inline std::ostream& operator<<(std::ostream& ost, const NewsClientCommand& cmd)
{
	return cmd.print(ost);
}

}

#endif
