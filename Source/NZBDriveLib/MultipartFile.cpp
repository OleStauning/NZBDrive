/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#include "MultipartFile.hpp"
#include <atomic>
#include <algorithm>

namespace ByteFountain
{

	MultipartFile::MultipartFile(Logger& log, const std::filesystem::path& path, const std::filesystem::path& filename):
		m_log(log),
		m_path(path),
		m_filename(filename),
		m_parts(),
		m_parts_map(),
		m_filesize(0)
	{
	}

	MultipartFile::~MultipartFile()
	{
		m_parts.clear();
		m_parts_map.clear();
	}

	std::filesystem::path MultipartFile::GetFileName()
	{
		return m_filename;
	}
	unsigned long long MultipartFile::GetFileSize()
	{
		return m_filesize;
	}

	bool MultipartFile::GetFileData(char* buf, const unsigned long long offset, const std::size_t size, std::size_t& readsize)
	{
		readsize=0;
		size_t r,rs;
		unsigned long long o=offset;
		size_t s=size;
		bool err=false;
		
		for(PartsMap::const_iterator i=m_parts_map.upper_bound(offset);
			readsize<size && i!=m_parts_map.end();
			++i)
		{
			const FilePart& part(i->second);
			unsigned long long e=i->first;
			unsigned long long b=e-part.size;
			assert(b<=o && o<e);
			r=std::min(s,(size_t)(e-o));
			err |= part.file->GetFileData(buf,o-b+part.offset,r,rs);
			readsize+=rs;
			err |= (rs<r); // truncated???? - no more data
			buf+=rs;
			o+=rs;
			s-=rs;
		}
		return err;
	}
	void MultipartFile::_AsyncGetFileData(const OnDataFunction& func, char* buf, const unsigned long long offset, const std::size_t size_in,
		CancelSignal* cancel, const bool priority)
	{
		auto size = std::min(size_in, (std::size_t)(m_filesize-offset));
		std::size_t r;
		unsigned long long o=offset;
		std::size_t s=size;
		auto readsize = std::make_shared< std::atomic<std::size_t> >(0);
		
		for(PartsMap::const_iterator i=m_parts_map.upper_bound(offset); s>0 && i!=m_parts_map.end(); ++i)
		{
			const FilePart& part(i->second);
			unsigned long long end=i->first;
			unsigned long long begin=end-part.size;
			assert(begin<=o && o<end);
			r=std::min(s,(size_t)(end-o));
			part.file->AsyncGetFileData(
				[this, r, readsize, size, func](const std::size_t rs)
				{
					if (rs<r)
					{
						m_log<<Logger::Error<<"MultipartFile::AsyncGetFileData (readsize<r)"<<Logger::End;
					}
					*readsize+=r;
					if (size==*readsize) func(*readsize);
				},
				buf,o-begin+part.offset,r,cancel,priority
			);
			if (buf!=0) buf+=r;
			o+=r;
			s-=r;
		}
	}
	
	// NEW BUFFERING:
	unsigned long long MultipartFile::CountMissingBytesInRange(const unsigned long long begin, const unsigned long long end)
	{
		PartsMap::const_iterator itBegin = m_parts_map.upper_bound(begin);
		PartsMap::const_iterator itLast = m_parts_map.lower_bound(end);
		PartsMap::const_iterator itEnd = itLast;
		if (itEnd != m_parts_map.end()) ++itEnd;

		unsigned long long count = 0;

		for (PartsMap::const_iterator it = itBegin; it != itEnd; ++it)
		{
			const MultipartFile::FilePart& part(it->second);

			const unsigned long long sub_begin =
				it == itBegin ? begin - (it->first - part.size) + part.offset :
				/*it != itBegin*/ part.offset;

			const unsigned long long sub_end =
				it == itLast ? end - (it->first - part.size) + part.offset :
				/*it != itBegin*/ part.offset + part.size;

			count += part.file->CountMissingBytesInRange(sub_begin, sub_end);
		}
		return count;

	}

	bool MultipartFile::BufferNextSegmentInRange(const unsigned long long begin, const unsigned long long end)
	{
		PartsMap::const_iterator itBegin=m_parts_map.upper_bound(begin);
		PartsMap::const_iterator itLast=m_parts_map.lower_bound(end);
		PartsMap::const_iterator itEnd = itLast;
		if (itEnd != m_parts_map.end()) ++itEnd;

		for(PartsMap::const_iterator it=itBegin;it!=itEnd;++it)
		{
			const MultipartFile::FilePart& part(it->second);
			
			const unsigned long long sub_begin = 
				  it == itBegin ? begin - (it->first - part.size) + part.offset :
				/*it != itBegin*/ part.offset;
			
			const unsigned long long sub_end =
				  it == itLast ? end - (it->first - part.size) + part.offset :
				/*it != itBegin*/ part.offset + part.size;
			
			if (part.file->BufferNextSegmentInRange(sub_begin,sub_end)) return true;
		}
		return false;
	}

	void MultipartFile::Finalize()
	{
		unsigned long long b=0,e=0;
		for(auto p : m_parts)
		{
			FilePart& part(p.second);
			m_log<<Logger::Debug<<"File: "<<part.file->GetFileName()<<" offset="<<part.offset<<" size="<<part.size<<Logger::End;

			e=b+part.size;
			m_parts_map[e]=part;
			b=e;
			SetErrorFlags(part.file->GetErrorFlags());
		}
		
		m_filesize=e;
	}
	
	bool MultipartFile::SetFilePart(const int part, std::shared_ptr<InternalFile> file, const unsigned long long offset, 
		const unsigned long long size, const bool bof, const bool eof)
	{
		if (m_parts.find(part) != m_parts.end())
		{
			m_log<<Logger::Debug<<"Filepart "<<file->GetFileName()<<" ("<<part<<") duplicate"<<Logger::End;
			return true;
		}
		
		m_parts[part].Set(file,offset,size,bof,eof);

		const FilePart& first(m_parts.begin()->second);
		unsigned int firstPart=m_parts.begin()->first;
		const FilePart& last(m_parts.rbegin()->second);
		unsigned int lastPart=m_parts.rbegin()->first;

		if (first.bof && last.eof && lastPart-firstPart==m_parts.size()-1)
		{
			m_log<<Logger::Debug<<"All parts of MultipartFile is found:"<<Logger::End;
			m_log<<Logger::Debug<<m_path/m_filename<<" consists of the following parts:"<<Logger::End;
			
			Finalize();

			if (m_filesize==0) return false; // TODO: What is the difference between empty file and directory??
			
			return true;
		}

		return false;
	}

}

