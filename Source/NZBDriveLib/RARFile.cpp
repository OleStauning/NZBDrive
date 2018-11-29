/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#include "RARFile.hpp"
#include "text_tool.hpp"
#include "assert.h"
#include <boost/xpressive/xpressive.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream> // DEBUG PURPOSE
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

	typedef unsigned char byte;   // unsigned 8 bits
	typedef wchar_t wchar;
	typedef unsigned int uint;   // 32 bits or more

	class RAREncodeFileName
	{
	private:
		void AddFlags(int Value);

		byte *EncName;
		byte Flags;
		uint FlagBits;
		size_t FlagsPos;
		size_t DestSize;
	public:
		RAREncodeFileName();
		void Decode(const char *Name, const byte *EncName, const std::size_t EncSize, wchar *NameW, const std::size_t MaxDecSize);
	};
	
	RAREncodeFileName::RAREncodeFileName()
	{
		Flags=0;
		FlagBits=0;
		FlagsPos=0;
		DestSize=0;
	}

	void RAREncodeFileName::Decode(const char *Name, const byte *EncName, const std::size_t EncSize, wchar *NameW,
		const std::size_t MaxDecSize)
	{
		std::size_t EncPos = 0, DecPos = 0;
		byte HighByte=EncName[EncPos++];
		while (EncPos<EncSize && DecPos<MaxDecSize)
		{
			if (FlagBits==0)
			{
				Flags=EncName[EncPos++];
				FlagBits=8;
			}
			switch(Flags>>6)
			{
			case 0:
				NameW[DecPos++]=EncName[EncPos++];
				break;
			case 1:
				NameW[DecPos++]=EncName[EncPos++]+(HighByte<<8);
				break;
			case 2:
				NameW[DecPos++]=EncName[EncPos]+(EncName[EncPos+1]<<8);
				EncPos+=2;
				break;
			case 3:
				{
				int Length=EncName[EncPos++];
				if (Length & 0x80)
				{
				byte Correction=EncName[EncPos++];
				for (Length=(Length&0x7f)+2;Length>0 && DecPos<MaxDecSize;Length--,DecPos++)
				NameW[DecPos]=((Name[DecPos]+Correction)&0xff)+(HighByte<<8);
				}
				else
				for (Length+=2;Length>0 && DecPos<MaxDecSize;Length--,DecPos++)
				NameW[DecPos]=Name[DecPos];
				}
				break;
			}
			Flags<<=2;
			FlagBits-=2;
		}
		NameW[DecPos<MaxDecSize ? DecPos:MaxDecSize-1]=0;
	}
}


	RARFile::RARFile(Logger& log, const std::filesystem::path& path, const std::filesystem::path& filename, 
		const rar::file_head& file_head): MultipartFile(log,path,filename), m_file_head(file_head)
	{
		if (InternalIsCompressed()) SetErrorFlag(FileErrorFlag::FileCompressed);
		if (InternalIsPWProtected()) SetErrorFlag(FileErrorFlag::FilePWProtected);		
	}
	RARFile::~RARFile()
	{
	}
	
	RARFileFactory::RARFileFactory(Logger& log, IDriveMounter& mounter):m_log(log),m_mounter(mounter){}
	
	bool RARFileFactory::GetRARFilenameAndPart(std::shared_ptr<InternalFile> file, std::string& name, int& part)
	{
		std::filesystem::path filename=file->GetFileName();
	
		std::string ext;
		smatch what;
		
		static sregex rarfile = sregex::compile("(?i:^(.*?)\\.((part\\d+\\.rar)|(rar)|(r\\d+))$)");
		
		std::string strFilename(filename.string());

		if (regex_match( strFilename, what, rarfile ))
		{
			name=what[1];
			ext=what[2];
			if (boost::iequals(ext,"rar")) part=0;
			else if (boost::istarts_with(ext,"r")) 
			try
			{
				part=1+std::stoi(ext.substr(1));
			}
			catch(std::exception& /*e*/)
			{
				m_log<<Logger::Fatal<<"Failed to parse integer from "<<ext.substr(1)<<" in GetRARFilenameAndPart, filename="<<filename<<Logger::End;
				throw;
			}
			else if (boost::istarts_with(ext,"part"))
			try
			{
				part=std::stoi(ext.substr(4));
			}
			catch(std::exception& /*e*/)
			{
				m_log<<Logger::Fatal<<"Failed to parse integer from "<<ext.substr(4)<<" in GetRARFilenameAndPart, filename="<<filename<<Logger::End;
				throw;
			}
			return true;
		}
		return false;
	}
	
	
	bool RARFileFactory::AddFile(const std::filesystem::path& path, std::shared_ptr<InternalFile> file, IFile::CancelSignal& cancel)
	{
		std::string name;
		int part=0;
		if (!GetRARFilenameAndPart(file,name,part)) return false;
		
		auto data = std::make_shared<rar_data>();
		data->err=FileErrorFlag::OK;
		data->rawpath=path;
		data->path=path/std::filesystem::path(name).stem();
		data->part=part;
		data->cancel=&cancel;
		GetMarkHead(data,file,0);
		
		return true;
	}
	
	void RARFileFactory::GetMainHeader(std::shared_ptr<rar_data> data, std::shared_ptr<InternalFile> file, const unsigned long long offset)
	{
		file->AsyncGetFileData(
			[this, file, data, offset, scope = m_mounter.NewPartScope()](const std::size_t readsize)
			{
//				std::cout<<"MAINHEAD "<<readsize<<std::endl;
				if (readsize!=sizeof(rar::main_head) || !rar::Validate(data->main_head))
				{
					m_log<<Logger::Error<<"Invalid RAR MAIN-HEADER"<<Logger::End;
//					rar::print(std::cout,data->main_head);
					data->err=FileErrorFlag::InvalidRARFile;
					return;
				}
				GetMarkHead(data,file,offset+readsize);
			}
		,(char*)&data->main_head,offset,sizeof(rar::main_head),data->cancel,true );
	}

	void RARFileFactory::GetFileHeader(std::shared_ptr<rar_data> data, std::shared_ptr<InternalFile> file, const unsigned long long offset, const unsigned char header_type)
	{
		file->AsyncGetFileData(
			[this, file, data, offset, header_type, scope = m_mounter.NewPartScope()](const std::size_t readsize)
			{
//				std::cout<<"FILEHEAD "<<readsize<<std::endl;
				if (readsize!=sizeof(rar::file_head) || !rar::Validate(data->file_head,header_type))
				{
					m_log<<Logger::Error<<"Invalid RAR FILE-HEADER"<<Logger::End;
//					rar::print(std::cout,data->file_head);
					data->err=FileErrorFlag::InvalidRARFile;
					return;
				}
				
				data->data_begin=offset+data->file_head.head_size;
				
				if (data->file_head.head_flags&0x100)
				{
					GetHighSize(data,file,offset+readsize,header_type);
				}
				else
				{
					GetFilename(data,file,offset+readsize,header_type);
				}

			}
		,(char*)&data->file_head,offset,sizeof(rar::file_head),data->cancel,true );
	}

	void RARFileFactory::GetHighSize(std::shared_ptr<rar_data> data, std::shared_ptr<InternalFile> file, const unsigned long long offset, 
		const unsigned char header_type)
	{
		file->AsyncGetFileData(
			[this, file, data, offset, header_type, scope = m_mounter.NewPartScope()](const std::size_t readsize)
			{
				if (readsize!=sizeof(rar::high_size))
				{
					m_log<<Logger::Error<<"Invalid RAR HIGH-SIZE"<<Logger::End;
					data->err=FileErrorFlag::InvalidRARFile;
					return;
				}
				GetFilename(data,file,offset+readsize,header_type);
			}
		,(char*)&data->high_size,offset,sizeof(rar::high_size),data->cancel,true );
	}
	
	void RARFileFactory::GetFilename(std::shared_ptr<rar_data> data, std::shared_ptr<InternalFile> file, const unsigned long long offset, 
		const unsigned char header_type)
	{
		data->txtfilename.resize(data->file_head.name_size);
		file->AsyncGetFileData(
			[this, file, data, offset, header_type, scope = m_mounter.NewPartScope()](const std::size_t readsize)
			{
				if (readsize!=data->file_head.name_size)
				{
//					std::cout<<"ERROR: Invalid FILENAME"<<std::endl;
					data->err=FileErrorFlag::InvalidRARFile;
					return;
				}
//				std::cout<<"FILENAME="<<data->filename<<std::endl;
				
//				std::cout<<"Data begins @ "<<data->data_begin<<std::endl;
//				std::cout<<"Data ends @ "<<data->data_begin+data->file_head.pack_size<<std::endl;
//				std::cout<<"File have size "<<file->GetFileSize()<<std::endl;
				
				if (header_type==0x74)
				{
				// file_head.head_flags
				// 0x200 - FILE_NAME contains both usual and encoded
				//        Unicode name separated by zero. In this case
				//        NAME_SIZE field is equal to the length
				//        of usual name plus encoded Unicode name plus 1.
				//
				//        If this flag is present, but FILE_NAME does not
				//        contain zero bytes, it means that file name
				//        is encoded using UTF-8.

					if (0!=(data->file_head.head_flags & 0x200))
					{
						size_t Length=strlen(data->txtfilename.c_str());
						if (Length<data->txtfilename.size())
						{
							wchar_t FileNameW[500];
							Length++;
							RAREncodeFileName NameCoder;
							NameCoder.Decode(data->txtfilename.c_str(),
								(byte *)data->txtfilename.c_str()+Length,
								data->txtfilename.size()-Length,FileNameW,
								sizeof(FileNameW)/sizeof(FileNameW[0]));
							data->filename=FileNameW;
						}
						else
						{
							std::replace( data->txtfilename.begin(), data->txtfilename.end(), '\\', '/');
							data->filename=data->txtfilename;						
						}
					}
					else
					{
						std::replace( data->txtfilename.begin(), data->txtfilename.end(), '\\', '/');
						data->filename=data->txtfilename;
					}

				
				
//					std::replace( data->filename.begin(), data->filename.end(), '\\', '/');
					data->filename.make_preferred();
					 
					std::filesystem::path filepath=data->path/data->filename;

					std::filesystem::path rar_path=filepath.parent_path();
					std::filesystem::path rar_name=filepath.filename();
					
					std::pair<std::filesystem::path,std::string> key(rar_path,rar_name.string());
					std::shared_ptr<RARFile>& rarFile = m_rar_files[key];
					if (rarFile.get()==0)
					{
						rarFile.reset(new RARFile(m_log,rar_path,rar_name.string(),data->file_head));
					}
				       
					bool bof=(data->file_head.head_flags&0x01)==0;
					bool eof=(data->file_head.head_flags&0x02)==0;

					if (rarFile->SetFilePart(data->part,file,data->data_begin,data->file_head.pack_size,bof,eof))
					{
						m_mounter.StartInsertFile(rarFile,rar_path);
						m_rar_files.erase(key);
					}
					
				}

				bool eof=(data->file_head.head_flags&0x02)==0;
				if (data->data_begin+data->file_head.pack_size < file->GetFileSize() && eof)
				{
//					std::cout<<"Get next rar-header"<<std::endl;
					GetMarkHead(data,file,data->data_begin+data->file_head.pack_size);
				}
			}
		,(char*)&data->txtfilename[0],offset,data->file_head.name_size,data->cancel,true );
		
	}
	
	
	void RARFileFactory::GetMarkHead(std::shared_ptr<rar_data> data, std::shared_ptr<InternalFile> file, const unsigned long long offset)
	{
		file->AsyncGetFileData(
			[this, file, data, offset, scope = m_mounter.NewPartScope()](const std::size_t readsize)
			{
//				std::cout<<"MARKHEAD "<<readsize<<std::endl;
				if (readsize!=sizeof(rar::mark_head)) // NOTE: DO not validate mark-head since it can be another headertype.
				{
					m_log<<Logger::Error<<"Invalid RAR MARK-HEADER"<<Logger::End;
					data->err=FileErrorFlag::InvalidRARFile;
					return;
				}
				switch(data->mark_head.header_type)
				{
					case /*HEAD_TYPE=*/0x72://          marker block
//						std::cout<<"RAR HEADER"<<std::endl;
						GetMarkHead(data,file,offset+readsize);
						break;
					case /*HEAD_TYPE=*/0x73://          main header
						if ((data->mark_head.head_flags&0x0080)!=0)
						{
							m_log<<Logger::Warning<<"RAR file is encrypted."<<Logger::End;
							file->SetPWProtected(true);
							file->SetErrorFlag(FileErrorFlag::FileEncrypted);
							m_mounter.RawInsertFile(file,data->rawpath);
						}
						else
						{
							GetMainHeader(data,file,offset);
						}
						break;
					case /*HEAD_TYPE=*/0x74://          file header
//						std::cout<<"FILE HEADER"<<std::endl;
						GetFileHeader(data,file,offset,data->mark_head.header_type);
						break;
					case /*HEAD_TYPE=*/0x75://          old style comment header
					case /*HEAD_TYPE=*/0x76://          old style authenticity information
					case /*HEAD_TYPE=*/0x77://          old style subblock
					case /*HEAD_TYPE=*/0x78://          old style recovery record
					case /*HEAD_TYPE=*/0x79://          old style authenticity information
						m_log<<Logger::Error<<"OLD RAR FILE WITH HEAD_TYPE="<<std::hex<<
						(int)data->mark_head.header_type<<std::dec<<Logger::End;
						break;
					case /*HEAD_TYPE=*/0x7a://          subblock
//						std::cout<<"Sub block ( FILEHEADER ?? )"<<std::endl;
						GetFileHeader(data,file,offset,data->mark_head.header_type);
						break;
					case /*HEAD_TYPE=*/0x7b://          archive end
//						std::cout<<"ARCHIVE END "<<offset+head->head_size<<std::endl;
						break;
					default:
						m_log<<Logger::Error<<"UNKNOWN HEADER TYPE 0x"<<
							std::hex<<(int)data->mark_head.header_type<<
							std::dec<<" AT OFFSET "<<offset<<Logger::End;
//						rar::print(std::cout,data->mark_head);
						break;
				}
			}
		,(char*)&data->mark_head,offset,sizeof(rar::mark_head),data->cancel,true );
		
	}

	void RARFileFactory::GetSubBlock(std::shared_ptr<rar_data> data, std::shared_ptr<InternalFile> file, const unsigned long long offset)
	{
		std::shared_ptr<rar::sub_block> sblock = std::shared_ptr<rar::sub_block>(new rar::sub_block());
		file->AsyncGetFileData(
			[this, file, data, sblock, offset, scope = m_mounter.NewPartScope()](const std::size_t readsize)
			{
				if (readsize!=sizeof(rar::sub_block) || !rar::Validate(*sblock))
				{
					m_log<<Logger::Error<<"Invalid RAR SUB-BLOCK"<<Logger::End;
//					rar::print(std::cout,*sblock.get());
					data->err=FileErrorFlag::InvalidRARFile;
					return;
				}
				
				unsigned long long offset1=offset+sizeof(rar::sub_block)+sblock->data_size;
				
//				std::cout<<"New offset="<<offset1<<std::endl;
				
				if (offset1 < file->GetFileSize())
				{
					GetMarkHead(data,file,offset1);
				}
				
			}
		,(char*)sblock.get(),offset,sizeof(rar::sub_block),data->cancel,true );
		
	}

	void RARFileFactory::Finalize()
	{
		m_rar_files.clear();
	}
	
}
