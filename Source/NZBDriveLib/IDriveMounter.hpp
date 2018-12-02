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
#include <filesystem>

namespace ByteFountain
{
	
class NZBDriveMounterScope;	
	
struct IDriveMounter 
{
	virtual ~IDriveMounter(){}
	virtual void RawInsertFile(std::shared_ptr<InternalFile> file, const std::filesystem::path& dir)=0;
	virtual void StartInsertFile(std::shared_ptr<InternalFile> file, const std::filesystem::path& dir)=0;
	virtual NZBDriveMounterScope NewPartScope()=0;
	virtual void PartIdentified()=0;
	virtual void PartFinalized()=0;
};

class NZBDriveMounterScope 
{
private:
	mutable std::shared_ptr<IDriveMounter> m_sharedptr;
public:
	NZBDriveMounterScope():m_sharedptr()
	{
	}
	NZBDriveMounterScope(const NZBDriveMounterScope& other):
		m_sharedptr(std::move(other.m_sharedptr))
	{
	}
	NZBDriveMounterScope(NZBDriveMounterScope&& other):
		m_sharedptr(std::move(other.m_sharedptr))
	{
	}
	NZBDriveMounterScope& operator=(NZBDriveMounterScope&& other)
	{
		m_sharedptr = std::move(other.m_sharedptr);
		return *this;
	}
	NZBDriveMounterScope(const std::shared_ptr<IDriveMounter>& sharedptr):m_sharedptr(sharedptr)
	{
		m_sharedptr->PartIdentified();
	}
	~NZBDriveMounterScope()
	{
		if (m_sharedptr) m_sharedptr->PartFinalized();
	}
};
	

}

#endif


