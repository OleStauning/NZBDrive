/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef INTERNALFILEVISITOR_FILE
#define INTERNALFILEVISITOR_FILE

#include "InternalFile.hpp"

namespace ByteFountain
{
	class NZBFile;
	class MultipartFile;
	
	class InternalFileVisitor
	{
	public:
		virtual void Visit(const NZBFile&) =0;
		virtual void Visit(const MultipartFile&) =0;
	};
	
}

#endif