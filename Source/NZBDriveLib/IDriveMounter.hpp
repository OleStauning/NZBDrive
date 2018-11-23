/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef IDRIVEMOUNTER_H
#define IDRIVEMOUNTER_H

#include <memory>
#include "InternalFile.hpp"
#include <boost/filesystem.hpp>

namespace ByteFountain
{

struct IDriveMounter 
{
		virtual ~IDriveMounter(){}
		virtual void StopInsertFile()=0;
		virtual void RawInsertFile(std::shared_ptr<InternalFile> file, const boost::filesystem::path& dir)=0;
		virtual void StartInsertFile(std::shared_ptr<InternalFile> file, const boost::filesystem::path& dir)=0;
//		virtual std::shared_ptr<IDriveMounter> GetSharedPtr()=0;
};

}

#endif


