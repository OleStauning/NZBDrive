/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#include "ZIPFile.hpp"
#include "text_tool.hpp"
#include "assert.h"
#include <boost/xpressive/xpressive.hpp>
#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <atomic>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace ByteFountain
{

using namespace boost;
using namespace boost::xpressive;
	
namespace
{
	bool GetZIPFilenameAndPart(std::shared_ptr<InternalFile> file, std::string& name, int& part)
	{
		std::filesystem::path filename=file->GetFileName();
		
		static sregex zipfile = sregex::compile("(?i:^(.*?)\\.(zip)$)");
		
		smatch what;

		std::string strFilename(filename.string());

		if (regex_match( strFilename, what, zipfile ))
		{
			name=what[1];
			part=1;
			return true;
		}
		return false;
	}
	
}
		
	ZIPFile::ZIPFile(Logger& log, const std::filesystem::path& path, const std::string& filename, const zip::local_file_header& local_file_header):
		MultipartFile(log,path,filename), m_local_file_header(local_file_header)
	{
		if (InternalIsCompressed()) SetErrorFlag(FileErrorFlag::FileCompressed);
		if (InternalIsPWProtected()) SetErrorFlag(FileErrorFlag::FilePWProtected);		
	}
	ZIPFile::~ZIPFile()
	{
	}

	
	ZIPFileFactory::ZIPFileFactory(Logger& log, IDriveMounter& mounter):m_log(log),m_mounter(mounter){}
	
	bool ZIPFileFactory::AddFile(const std::filesystem::path& path, std::shared_ptr<InternalFile> file, IFile::CancelSignal& cancel)
	{
		std::string name;
		int part=0;
		if (!GetZIPFilenameAndPart(file,name,part)) return false;
		
		std::shared_ptr<zip_data> data(new zip_data());
		data->err=false;
		data->path=path/std::filesystem::path(name).stem();
		data->part=part;
		data->cancel=&cancel;
		GetZIPLocalFileHeader(data,file,0);
		
		return true;
	}
	
	
	void ZIPFileFactory::GetZIPLocalFileHeader(std::shared_ptr<zip_data> data, std::shared_ptr<InternalFile> file, 
		const unsigned long long offset)
	{
		file->AsyncGetFileData(
			[this, file, data, offset](const std::size_t readsize)
			{
				if (readsize!=sizeof(zip::local_file_header) || !zip::Validate(data->local_file_header))
				{
					m_log<<Logger::Error<<"Invalid ZIP-LOCAL-FILE-HEADER"<<Logger::End;
//					zip::print(std::cout,data->local_file_header);
					data->err=true;
					m_mounter.StopInsertFile();
					return;
				}
				zip::print(std::cout,data->local_file_header);
				
				data->data_begin=offset+sizeof(zip::local_file_header)+
					+data->local_file_header.file_name_length
					+data->local_file_header.extra_field_length;
				
				GetFilename(data,file,offset+sizeof(zip::local_file_header));
			}
		,(char*)&data->local_file_header,offset,sizeof(zip::local_file_header),data->cancel,true );
	}

	void ZIPFileFactory::GetFilename(std::shared_ptr<zip_data> data, std::shared_ptr<InternalFile> file, const unsigned long long offset)
	{
		data->filename.resize(data->local_file_header.file_name_length);
		file->AsyncGetFileData(
			[this, file, data, offset](const std::size_t readsize)
			{
				if (readsize!=data->local_file_header.file_name_length)
				{
					m_log<<Logger::Error<<"Invalid ZIP FILENAME"<<Logger::End;
					data->err=true;
					m_mounter.StopInsertFile();
					return;
				}
//				std::cout<<"FILENAME="<<data->filename<<std::endl;
				
				unsigned long long data_end=data->data_begin+data->local_file_header.compressed_size;
				
//				std::cout<<"Data begins @ "<<data->data_begin<<std::endl;
//				std::cout<<"Data ends @ "<<data_end<<std::endl;
//				std::cout<<"File have size "<<file->GetFileSize()<<std::endl;

				std::replace( data->filename.begin(), data->filename.end(), '\\', '/');
				
				data->filename = text_tool::cp_to_utf8(text_tool::cp1252, data->filename);
				
				std::filesystem::path filepath=data->path/data->filename;
				
				std::filesystem::path zip_path=filepath.parent_path();
				std::filesystem::path zip_name=filepath.filename();

				auto key=std::pair<std::filesystem::path,std::string>(zip_path,zip_name.string());
				std::shared_ptr<ZIPFile>& zipFile=m_zip_files[key];
				if (zipFile.get()==0)
				{
					zipFile.reset(new ZIPFile(m_log,zip_path,zip_name.string(),data->local_file_header));
				}
				
				bool bof=true;
				bool eof=true;

				if (zipFile->SetFilePart(data->part,file,data->data_begin,data_end,bof,eof))
				{
					m_mounter.StartInsertFile(zipFile,zip_path);
					m_zip_files.erase(key);
				}
				
				GetZIPNextFileHeader(data,file,data_end);
			}
		,(char*)&data->filename[0],offset,data->local_file_header.file_name_length,data->cancel,true );
	}

	void ZIPFileFactory::GetZIPNextFileHeader(std::shared_ptr<zip_data> data, std::shared_ptr<InternalFile> file, const unsigned long long offset)
	{
		file->AsyncGetFileData(
			[this, file, data, offset](const std::size_t readsize)
			{
				if (readsize!=sizeof(unsigned int))
				{
					m_log<<Logger::Error<<"Invalid ZIP HEADER_SIGNATURE"<<Logger::End;
					data->err=true;
					m_mounter.StopInsertFile();
					return;
				}
				if (data->next_header_signature==0x04034b50)
				{
					GetZIPLocalFileHeader(data,file,offset);
				}
				else
				{
					m_mounter.StopInsertFile();
				}
				
			}
		,(char*)&data->next_header_signature,offset,sizeof(unsigned int),data->cancel,true );
	}

	void ZIPFileFactory::Finalize()
	{
		m_zip_files.clear();
	}
	
}
