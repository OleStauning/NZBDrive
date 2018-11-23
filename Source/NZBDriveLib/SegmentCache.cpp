/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/


#include "SegmentCache.hpp"
#include <sstream>

namespace ByteFountain
{

namespace 
{
template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}	
}


CachedSegment::CachedSegment():Allocated(0),Size(0),Data(nullptr){}
CachedSegment::CachedSegment(size_t newSize):Allocated(newSize),Size(newSize),Data(malloc(newSize)){}
CachedSegment::~CachedSegment(){ if (Data!=nullptr) free(Data); }
void CachedSegment::ReAllocate(size_t newSize)
{
	if (newSize>Allocated)
	{
		free(Data);
		Allocated = newSize;
		Data = malloc(Allocated);
	}
	Size=newSize;
}
void CachedSegment::Read(void *buf, const unsigned long long offset, const std::size_t size)
{
	if (offset+size>Size)
	{
		std::ostringstream oss;
		oss<<"CachedSegment::Read: Attempt to read "<<size<<" bytes from offset "<<offset<<" in buffer with length "<<Size;
		throw std::out_of_range(oss.str());
	}
	std::memcpy(buf,(char*)Data+offset,size);
}
void CachedSegment::Write(const void *buf, const unsigned long long offset, const std::size_t size)
{
	if (offset+size>Size)
	{
		std::ostringstream oss;
		oss<<"CachedSegment::Write: Attempt to write "<<size<<" bytes from offset "<<offset<<" in buffer with length "<<Size;
		throw std::out_of_range(oss.str());
	}
	std::memcpy((char*)Data+offset,buf,size);
}

SegmentCache::Key::Key(int32_t fId, int sIdx):FileId(fId),SegmentIdx(sIdx){}

bool SegmentCache::Key::operator==(const Key& k) const { return FileId == k.FileId && SegmentIdx == k.SegmentIdx; }

std::size_t SegmentCache::Key::Hash::operator() (const Key& k) const
{  
	std::size_t seed = 0;
	hash_combine(seed, k.FileId);
	hash_combine(seed, k.SegmentIdx);
	return seed;
} 

SegmentCache::SegmentCache(Logger& logger):m_logger(logger){}

std::shared_ptr<CachedSegment> SegmentCache::TryGet(int32_t fileId, int segmentIdx)
{
//	m_logger << Logger::Info << "SegmentCache.TryGet("<<fileId<<", "<<segmentIdx<<")"<< Logger::End;
	std::lock_guard<std::mutex> guard(m_mutex);
	auto it = m_segments.find(Key(fileId,segmentIdx));
	if (it != m_segments.end()) return it->second;
	return nullptr;
}

std::shared_ptr<CachedSegment> SegmentCache::Create(int32_t fileId, int segmentIdx, const std::size_t size)
{
//	m_logger << Logger::Info << "SegmentCache.Create("<<fileId<<", "<<segmentIdx<<", "<<size<<")" << Logger::End;
	std::lock_guard<std::mutex> guard(m_mutex);
	
	std::shared_ptr<CachedSegment> segment;
	
	if (m_fifo_keys.size()>30)
	{
		for(auto itkey = m_fifo_keys.begin(); itkey != m_fifo_keys.end(); ++itkey)
		{
			const auto itseg = m_segments.find(*itkey);
			const auto& segptr = itseg->second;
			m_logger << Logger::Info << "   Looking for unused segment key=("<<itkey->FileId<<", "<<itkey->SegmentIdx<<") Usage "<< segptr.use_count() << Logger::End;
			if (segptr.use_count() == 1)
			{
				segment = segptr;
				segment->ReAllocate(size);
				m_fifo_keys.erase(itkey);
				m_segments.erase(itseg);
				break;
			}
		}
	}
	
	if (!segment) segment = std::make_shared<CachedSegment>(size);
	auto key = Key(fileId,segmentIdx);
	m_segments[key] = segment;
	m_fifo_keys.push_back(key);
	return segment;
}

}

