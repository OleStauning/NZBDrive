/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef NZB_FILELOADER_HPP
#define NZB_FILELOADER_HPP
#include <string>

namespace ByteFountain
{
	struct nzb;
	nzb loadnzb(const std::string& location);
}

#endif
