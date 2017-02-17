/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef ZIP_FILE
#define ZIP_FILE
#include "MultipartFile.hpp"
#include "IDriveMounter.hpp"
#include "Logger.hpp"
#include <boost/filesystem.hpp>
#include <map>
#include <memory>
#include <ostream>
#include "zip_structs.hpp"

namespace ByteFountain
{

class ZIPFile : public MultipartFile
{
	friend class ZIPFileFactory;
	zip::local_file_header m_local_file_header;

public:
	ZIPFile(const ZIPFile&) = delete;
	ZIPFile& operator=(const ZIPFile&) = delete;

	ZIPFile(Logger& log, const boost::filesystem::path& path, const std::string& filename, const zip::local_file_header& local_file_header);
	virtual ~ZIPFile();
	
	bool InternalIsCompressed() const { return m_local_file_header.compression_method!=0; }
	bool InternalIsPWProtected() const { return (m_local_file_header.general_purpose_flags&0x01)!=0; }
};
	
	
class ZIPFileFactory
{
	Logger& m_log;
	IDriveMounter& m_mounter;
	
	typedef std::map<std::pair<boost::filesystem::path,std::string>, std::shared_ptr<ZIPFile> > ZIPFileMap;
	ZIPFileMap m_zip_files;
	
	struct zip_data
	{
		boost::filesystem::path path;
		int part;
		bool err;
		zip::local_file_header local_file_header;
		std::string filename;
		unsigned long long data_begin;
		unsigned int next_header_signature;
		std::shared_ptr<IDriveMounter> mounter;
		IFile::CancelSignal* cancel;
	};

public:
	ZIPFileFactory(Logger& log, IDriveMounter& mounter);
	
	bool AddFile(const boost::filesystem::path& path, std::shared_ptr<InternalFile> file, IFile::CancelSignal& cancel);
	
	void GetZIPLocalFileHeader(std::shared_ptr<zip_data> data, std::shared_ptr<InternalFile> file, const unsigned long long offset);
	void GetFilename(std::shared_ptr<zip_data> data, std::shared_ptr<InternalFile> file, const unsigned long long offset);
	void GetZIPNextFileHeader(std::shared_ptr<zip_data> data, std::shared_ptr<InternalFile> file, const unsigned long long offset);

	void Finalize();
};

}
#endif
