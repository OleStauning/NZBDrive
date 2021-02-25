/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef IOVERVIEWRESPONSEHANDLER_HPP
#define IOVERVIEWRESPONSEHANDLER_HPP

#include <string>

namespace ByteFountain
{
	struct OverviewResponse
	{
		std::string Response;
		std::string ID; // 0
		std::string Subject; // 1
		std::string From; // 2
		std::string Date; // 3
		std::string MessageID; // 4
		std::string Bytes; // 6
		std::string Lines; // 7
		std::string References; // 8
		
		OverviewResponse()
		{}
		
		OverviewResponse(
			const std::string response,
			const std::string id,
			const std::string subject,
			const std::string from,
			const std::string date,
			const std::string messageID,
			const std::string bytes,
			const std::string lines,
			const std::string references):
			Response(response),
			ID(id),
			Subject(subject),
			From(from),
			Date(date),
			MessageID(messageID),
			Bytes(bytes),
			Lines(lines),
			References(references)
		{}
	};
	
	
	struct IOverviewResponseHandler
	{
		virtual ~IOverviewResponseHandler(){}
		virtual void OnErrorRetry() = 0;
		virtual void OnCancel() = 0;
		virtual void OnErrorCancel() = 0;
		virtual void OnGroupResponse(const bool groupExists, const unsigned long long size, 
			const unsigned long long low, const unsigned long long high) = 0;
		virtual void OnOverviewResponse(const OverviewResponse& line) = 0;
		virtual void OnEnd(const bool EOM) = 0;
	};
}

#endif
