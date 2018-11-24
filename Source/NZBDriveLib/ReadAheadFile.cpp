/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#include "ReadAheadFile.hpp"
#include "NZBDriveIMPL.hpp"

namespace ByteFountain
{
//	typedef boost::signals2::signal<void ()> CancelSignal;
	
	ReadAheadFile::ReadAheadFile(boost::asio::io_service& io_service, Logger& logger, 
				     std::shared_ptr<InternalFile> file, NZBDriveIMPL& drive):
		m_io_service(io_service),
		m_strand(m_io_service),
		m_logger(logger),
		m_file(file),
		m_drive(drive),
		m_filesize(m_file->GetFileSize()),
		m_offset(0)
	{
	}
	
	ReadAheadFile::~ReadAheadFile(){};
	bool ReadAheadFile::IsCompressed() const { return m_file->IsCompressed(); }
	bool ReadAheadFile::IsPWProtected() const { return m_file->IsPWProtected(); }
	std::filesystem::path ReadAheadFile::GetFileName() { return m_file->GetFileName(); }
	unsigned long long ReadAheadFile::GetFileSize() { return m_file->GetFileSize(); }
	bool ReadAheadFile::GetFileData(char* buf, const unsigned long long offset, const std::size_t size, std::size_t& readsize)
	{
		m_drive.StartRead(size);
		
		unsigned long long missingBytes = CountMissingBytesInRange(offset, offset + size + m_drive.m_shape.FastPreCache);

		m_offset = offset;

		m_drive.PrecacheMissingBytes(shared_from_this(), missingBytes);
		
		bool res = m_file->GetFileData(buf,offset,size,readsize);
		
		m_drive.StopRead();
		
		return res;
	}

	void ReadAheadFile::AsyncGetFileData(OnDataFunction func, char* buf, const unsigned long long offset, const std::size_t size,
		CancelSignal* cancel = 0, const bool priority=false)
	{
		m_file->AsyncGetFileData(func, buf, offset, size, nullptr, priority);
	}
	
	unsigned long long ReadAheadFile::CountMissingBytesInRange(const unsigned long long begin, const unsigned long long end)
	{
		return m_file->CountMissingBytesInRange(begin, end);
	}

	void ReadAheadFile::BufferNextSegment()
	{
		if (m_unmounted) m_drive.PrecacheMissingBytes(nullptr, 0);
		else BufferNextSegmentInRange(m_offset, m_filesize);
	}

	bool ReadAheadFile::BufferNextSegmentInRange(const unsigned long long begin, const unsigned long long end)
	{
		return m_file->BufferNextSegmentInRange(begin,end);
	}



}

