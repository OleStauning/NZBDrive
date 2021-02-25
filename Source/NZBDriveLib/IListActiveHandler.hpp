/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef ILISTACTIVEHANDLER_HPP
#define ILISTACTIVEHANDLER_HPP

#include <string>

namespace ByteFountain
{
	struct IListActiveHandler
	{
		virtual ~IListActiveHandler(){}
		virtual void OnActiveGroupInfo(const std::string& response, const std::string& group, const std::string& high, const std::string& low, const std::string& allowed) = 0;
	};

}

#endif
