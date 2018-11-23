/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef SEGMENTCACHE_HPP
#define SEGMENTCACHE_HPP

#include <unordered_map>
#include <cstring>
#include <memory>
#include <mutex>
#include <deque>
#include "Logger.hpp"

namespace ByteFountain
{
	
struct CachedSegment
{
	size_t Allocated;
	size_t Size;
	void* Data;
public:
	CachedSegment();
	CachedSegment(size_t newSize);
	~CachedSegment();
	void ReAllocate(size_t newSize);
	void Read(void *buf, const unsigned long long offset, const std::size_t size);
	void Write(const void *buf, const unsigned long long offset, const std::size_t size);
};

class SegmentCache
{
	struct Key
	{
		int32_t FileId;
		int SegmentIdx;
	public:
		Key(int32_t fId, int sIdx);
		bool operator==(const Key& k) const;
		struct Hash 
		{
			std::size_t operator() (const Key& k) const;
		};
	};
	
	Logger& m_logger;
	std::mutex m_mutex;
	std::unordered_map< Key, std::shared_ptr<CachedSegment>, SegmentCache::Key::Hash > m_segments;
	std::deque< Key > m_fifo_keys;
public:
	
	SegmentCache(Logger& logger);
	std::shared_ptr<CachedSegment> TryGet(int32_t fileId, int segmentIdx);
	std::shared_ptr<CachedSegment> Create(int32_t fileId, int segmentIdx, const std::size_t size);
};

}
#endif
