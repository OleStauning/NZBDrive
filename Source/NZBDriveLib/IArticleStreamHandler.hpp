/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef IARTICLESTREAMHANDLER_HPP
#define IARTICLESTREAMHANDLER_HPP

namespace ByteFountain
{

struct IArticleStreamHandler
{
	virtual ~IArticleStreamHandler(){}
	virtual void OnErrorRetry() = 0;
	virtual void OnCancel() = 0;
	virtual void OnErrorCancel() = 0;
	virtual void OnData(const char* line, const std::size_t len) = 0;
	virtual void OnEnd() = 0;
};

}

#endif
