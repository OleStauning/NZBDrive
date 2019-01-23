/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef READAHEADFILE_H
#define READAHEADFILE_H

#include <boost/asio.hpp>
#include "InternalFile.hpp"
#include "Logger.hpp"
#include <mutex>
#include <atomic>

namespace ByteFountain
{
	class NZBDriveIMPL;
	
	class ReadAheadFile : public InternalFile, public std::enable_shared_from_this<ReadAheadFile>
	{
		boost::asio::io_service& m_io_service;
		boost::asio::io_service::strand m_strand;
		Logger& m_logger;
		std::shared_ptr<InternalFile> m_file;
		NZBDriveIMPL& m_drive;
		unsigned long long m_filesize;
		std::atomic<unsigned long long> m_offset;

		virtual unsigned long long CountMissingBytesInRange(const unsigned long long begin, const unsigned long long end);
		virtual bool BufferNextSegmentInRange(const unsigned long long begin, const unsigned long long end);
	public:
		
		ReadAheadFile(boost::asio::io_service& io_service, Logger& logger, 
			      std::shared_ptr<InternalFile> file, NZBDriveIMPL& drive);

		virtual ~ReadAheadFile();
		bool IsCompressed() const;
		bool IsPWProtected() const;
		std::filesystem::path GetFileName();
		unsigned long long GetFileSize();
		bool GetFileData(char* buf, const unsigned long long offset, const std::size_t size, std::size_t& readsize);

		void _AsyncGetFileData(const OnDataFunction& func, char* buf, const unsigned long long offset, const std::size_t size,
			CancelSignal* cancel, const bool priority);
		
		void BufferNextSegment();

	};
}

#endif
