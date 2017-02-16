/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef RAR_FILE
#define RAR_FILE
#include "MultipartFile.hpp"
#include "IDriveMounter.hpp"
#include "Logger.hpp"
#include <boost/filesystem.hpp>
#include <map>
#include <memory>
#include <ostream>
#include "rar_structs.hpp"

namespace ByteFountain
{

class RARFile : public MultipartFile
{
private:
	friend class RARFileFactory;
	rar::file_head m_file_head;
	
public:
	RARFile(const RARFile&) = delete;
	RARFile& operator=(const RARFile&) = delete;

	
	RARFile(Logger& log, const boost::filesystem::path& path, const boost::filesystem::path& filename, const rar::file_head& file_head);
	virtual ~RARFile();	
	bool InternalIsCompressed() const { return m_file_head.method!=0x30; }
	bool InternalIsPWProtected() const { return (m_file_head.head_flags&0x04)!=0; }
};
	
	
class RARFileFactory
{
	Logger& m_log;
	IDriveMounter& m_mounter;
	
	typedef std::map<std::pair<boost::filesystem::path,std::string>, std::shared_ptr<RARFile> > RARFileMap;
	RARFileMap m_rar_files;
	
	struct rar_data
	{
		boost::filesystem::path rawpath;
		boost::filesystem::path path;
		int part;
		FileErrorFlag err;
		rar::mark_head mark_head;
		rar::main_head main_head;
		rar::file_head file_head;
		rar::high_size high_size;
		boost::filesystem::path filename;
		std::string txtfilename;
		unsigned long long data_begin;
		std::shared_ptr<IDriveMounter> mounter;
		IFile::CancelSignal* cancel;
	};

public:
	RARFileFactory(Logger& log, IDriveMounter& mounter);
	
	bool GetRARFilenameAndPart(std::shared_ptr<InternalFile> file, std::string& name, int& part);
	bool AddFile(const boost::filesystem::path& path, std::shared_ptr<InternalFile> file, IFile::CancelSignal& cancel);
	
	void GetMainHeader(std::shared_ptr<rar_data> data, std::shared_ptr<InternalFile> file, const unsigned long long offset);
	void GetFileHeader(std::shared_ptr<rar_data> data, std::shared_ptr<InternalFile> file, const unsigned long long offset, 
		const unsigned char header_type);
	void GetHighSize(std::shared_ptr<rar_data> data, std::shared_ptr<InternalFile> file, const unsigned long long offset, 
		const unsigned char header_type);
	void GetFilename(std::shared_ptr<rar_data> data, std::shared_ptr<InternalFile> file, const unsigned long long offset, 
		const unsigned char header_type);
	void GetMarkHead(std::shared_ptr<rar_data> data, std::shared_ptr<InternalFile> file, const unsigned long long offset);
	void GetSubBlock(std::shared_ptr<rar_data> data, std::shared_ptr<InternalFile> file, const unsigned long long offset);

	void Finalize();
};

}
#endif
