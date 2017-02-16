/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef NEWSSERVER_HPP
#define NEWSSERVER_HPP

#include <string>
#include <ostream>
#include <cstdint>

namespace ByteFountain
{

struct NewsServer
{
	std::string Servername;
	std::string Username;
	std::string Password;
	std::string Port;
	bool SSL;
	int Timeout;
	size_t Pipeline;

	struct ServerStatistics
	{
		uint64_t BytesTX;
		uint64_t BytesRX;
		uint32_t MissingSegmentCount;
		uint32_t ConnectionTimeoutCount;
		uint32_t ErrorCount;

		ServerStatistics() : BytesTX(0), BytesRX(0), MissingSegmentCount(0), ConnectionTimeoutCount(0), ErrorCount(0) {}
	};

	ServerStatistics Statistics;
};

inline std::ostream& operator<<(std::ostream& ost, const NewsServer& srv)
{
	ost<<"nntp";
	if (srv.SSL) ost<<"s";
	ost<<"://"<<srv.Username<<"@"<<srv.Servername<<":"<<srv.Port<<" ( timeout="<<srv.Timeout<<", pipeline="<<srv.Pipeline<<" )";
	return ost;
}

}

#endif