/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef CONNECTIONSTATE_HPP
#define CONNECTIONSTATE_HPP

namespace ByteFountain
{
	enum class ConnectionState 
	{ 
		Disconnected, 
		Connecting, 
		Idle, 
		Working 
	};
}

#endif