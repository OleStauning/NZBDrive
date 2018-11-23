/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef INTERNAL_FILE
#define INTERNAL_FILE
#include "IFile.hpp"

namespace ByteFountain
{
	enum class FileErrorFlag : unsigned long
	{
		OK = 0,
		// Warnings:
		FileEncrypted = 1 << 0,
		FileCompressed = 1 << 1,
		FilePWProtected = 1 << 2,
		// Errors:
		MissingSegment = 1 << 10,
		InvalidSegmentNumber = 1 << 11,
		InvalidSegmentSize = 1 << 12,
		EmptyMessageID = 1 << 13,
		CacheFileError = 1 << 14,
		ErrorInSegment = 1 << 15,
		DownloadCanceled = 1 << 16,
		DecodingFailed = 1 << 17,
		EmptySegment = 1 << 18,
		InvalidRARFile = 1 << 19
	};
	typedef unsigned long FileErrorFlags;
	
	
	class InternalFileVisitor;
	
	class InternalFile : public IFile
	{
		bool m_is_compressed;
		bool m_is_password_protected;
		FileErrorFlags m_errors;
	protected:
		bool m_unmounted;
	public:
		InternalFile(const InternalFile&) = delete;
		InternalFile& operator=(const InternalFile&) = delete;

		InternalFile() :m_is_compressed(false), m_is_password_protected(false), m_errors((FileErrorFlags)FileErrorFlag::OK), m_unmounted(false){}
		virtual ~InternalFile(){}
		virtual bool InternalIsCompressed() const { return false; }
		virtual bool InternalIsPWProtected() const { return false; }
		bool IsCompressed() const { return m_is_compressed?true:InternalIsCompressed(); }
		bool IsPWProtected() const { return m_is_password_protected?true:InternalIsPWProtected(); }
		void SetCompressed(const bool value){ m_is_compressed=value; }
		void SetPWProtected(const bool value){ m_is_password_protected=value; }
		void SetErrorFlag(const FileErrorFlag flag){ m_errors|=(FileErrorFlags)flag; }
		void SetErrorFlags(const FileErrorFlags flags){ m_errors|=flags; }
		FileErrorFlags GetErrorFlags() const { return m_errors; }
		bool HasError(){ return m_errors!=(FileErrorFlags)FileErrorFlag::OK; }
		void Unmount() { m_unmounted = true; }

		virtual unsigned long long CountMissingBytesInRange(const unsigned long long begin, const unsigned long long end)=0;
		virtual bool BufferNextSegmentInRange(const unsigned long long begin, const unsigned long long end)=0;

		virtual void Accept(InternalFileVisitor& v) const {}
		
	};

}

#endif
