/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef MULTIPART_FILE
#define MULTIPART_FILE

#include "InternalFile.hpp"
#include "InternalFileVisitor.hpp"
#include "Logger.hpp"
#include <filesystem>
#include <map>

namespace ByteFountain
{

class MultipartFile : public InternalFile
{
private:
	
	friend class MultipartFileSegmentBuffering;
	
	struct FilePart
	{
		std::shared_ptr<InternalFile> file; // Underlying "file"
		unsigned long long offset;          // Offset in "file" where part begins
		unsigned long long size;            // Size of part.
		bool bof;                           // First part?
		bool eof;                           // Last Part?
		FilePart():file(),offset(0),size(0),bof(false),eof(false){}
		void Set(std::shared_ptr<InternalFile> f, const unsigned long long o, const unsigned long long s, const bool b, const bool e)
		{
			file=f;offset=o;size=s;bof=b;eof=e;
		}
	};
	
	Logger& m_log;
	std::filesystem::path m_path;             // Path-name of multifile-part location
	std::filesystem::path m_filename;         // File-name of multipart-file
	std::map< int, FilePart > m_parts;          // Map for all file-parts: part-nuymber => file-part
	typedef std::map< unsigned long long, FilePart > PartsMap;
	PartsMap m_parts_map;                       // Map for all file-parts: part-end-address => file-part
	unsigned long long m_filesize;              // Size of multipart-file
	
public:
	MultipartFile(Logger& log, const std::filesystem::path& path, const std::filesystem::path& filename);
	virtual ~MultipartFile();
	std::filesystem::path GetFileName();
	unsigned long long GetFileSize();
	bool GetFileData(char* buf, const unsigned long long offset, const std::size_t size, std::size_t& readsize);
	void AsyncGetFileData(OnDataFunction func, char* buf, const unsigned long long offset, const std::size_t size, CancelSignal* cancel = 0, const bool priority = false);
	bool SetFilePart(const int part, std::shared_ptr<InternalFile> file, const unsigned long long offset, const unsigned long long size, const bool bof, const bool eof);
	void Finalize();

	virtual unsigned long long CountMissingBytesInRange(const unsigned long long begin, const unsigned long long end);
	virtual bool BufferNextSegmentInRange(const unsigned long long begin, const unsigned long long end);
	
	void Accept(InternalFileVisitor& visitor) const { visitor.Visit(*this); }

};

}

#endif
