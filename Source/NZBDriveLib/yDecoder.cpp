/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#include "yDecoder.hpp"
#include <stdlib.h>
#include <sstream>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>

#include <iostream>
#include <fstream>
#include <cstring>
#include <cstddef>

namespace ByteFountain
{
	using namespace boost::interprocess;

	struct fileregion
	{
		boost::interprocess::file_mapping& m_file;
		boost::interprocess::mapped_region m_region;
		char* m_cursorBegin;
		char* m_cursor;
		char* m_cursorEnd;
		fileregion(boost::interprocess::file_mapping& file, const boost::interprocess::mode_t mode, 
			const unsigned long long offset, const std::size_t size) :
			m_file(file),m_region(file,mode,offset,size),m_cursorBegin((char*)m_region.get_address()),
			m_cursor(m_cursorBegin),m_cursorEnd(m_cursor+size)
		{
//			std::cout<<m_file.get_name()<<"->Open(["<<offset<<","<<offset+size<<"[) : ["<<(size_t)m_cursorBegin<<","<<(size_t)m_cursorEnd<<"["<<std::endl;			
		}
		~fileregion()
		{
//			std::cout<<m_file.get_name()<<"->Close(["<<(size_t)m_cursorBegin<<","<<(size_t)m_cursorEnd<<"[)"<<std::endl;			
		}
		std::size_t WriteToCursor(void* buf, const std::size_t size)
		{
//			std::cout<<m_file.get_name()<<"->WriteToCursor("<<(size_t)m_cursor<<","<<size<<")"<<std::endl;

			if (m_cursor+size>m_cursorEnd)
			{
				memcpy((void*)m_cursor, buf, m_cursorEnd - m_cursor);
				m_cursor = m_cursorEnd;
				return m_cursorEnd - m_cursor;
//				std::ostringstream oss;
//				oss << "Writing " << (m_cursor + size) - m_cursorEnd << " bytes past end of memory mapped file, reading-size= "
//					<<size<<", filesize="<<m_cursorEnd-m_cursorBegin;
//				throw std::invalid_argument(oss.str());
			}
			memcpy((void*)m_cursor,buf,size);
			m_cursor+=size;
			return size;
		}
		std::size_t ReadFromCursor(void* buf, const std::size_t size)
		{
//			std::cout<<m_file.get_name()<<"->ReadFromCursor("<<(size_t)buf<<","<<size<<")"<<std::endl;

			if (m_cursor+size>m_cursorEnd)
			{
				memcpy(buf, (void*)m_cursor, m_cursorEnd - m_cursor);
				return m_cursorEnd - m_cursor;
//				std::ostringstream oss;
//				oss << "Reading "<<m_cursorEnd-(m_cursor+size)<<" bytes past end of memory mapped file (reading-offset= "
//					<<m_cursor-m_cursorBegin<<" reading-size= "<<size<<" filesize="<<m_cursorEnd-m_cursorBegin<<")";
			}
			memcpy(buf,(void*)m_cursor,size);
			return size;
		}
	};

	
	void yCacheFile::AllocateCacheFile(const std::string& path, const unsigned long long size)
	{
		m_path=path;
		try
		{
			std::filebuf fbuf;
			fbuf.open(m_path, std::ios_base::in | std::ios_base::out 
						| std::ios_base::trunc | std::ios_base::binary);

			//  Preallocate file of size:
			auto actual = fbuf.pubseekoff(size - 1, std::ios_base::beg);
			if (std::char_traits<char>::eof() != fbuf.sputc(0)) actual += std::streamoff(1);

			// For some reason "actual==size" even when filesystem is full....

			fbuf.close();

			if (size != boost::filesystem::file_size(path))
			{
				throw std::runtime_error("Cache file truncated.");
			}

			
			m_file=file_mapping(m_path.c_str(),read_write);
		}
		catch(std::exception &ex)
		{
			boost::filesystem::remove(m_path);
			std::ostringstream err;
			err<<"Unable to allocate cachefile: \""<<path<<"\" of size "<<size<<".\n"
				<<"Check for full disk and you have read-write-create access to the directory. "
				<<ex.what();
			throw std::runtime_error(err.str());
		}
	}
	void yCacheFile::OpenCacheFileForWrite(const unsigned long long offset, const std::size_t size, yCacheFileHandle& cacheFile)
	{
		try
		{
			cacheFile.reset(new fileregion(m_file,read_write,offset,size));
		}
		catch(std::exception &ex)
		{
			std::ostringstream err;
			err<<"Unable to open cachefile: \""<<m_path<<"\":\n"<<ex.what();
			throw std::runtime_error(err.str());
		}
	}
	void yCacheFile::CloseCacheFile(yCacheFileHandle& cacheFile)
	{
		cacheFile.reset();
	}
	std::size_t yCacheFile::WriteToCacheFile(const unsigned char* buf, const std::size_t size, yCacheFileHandle& cacheFile)
	{
		return cacheFile? cacheFile->WriteToCursor((void*)buf,size) : 0;
	}
	std::size_t yCacheFile::ReadFromCacheFile(char* buf, const unsigned long long offset, const std::size_t size)
	{
		fileregion region(m_file,read_only,offset,size);
		return region.ReadFromCursor((void*)buf,size);
	}

	yCacheFile::yCacheFile(){}
	yCacheFile::~yCacheFile()
	{
		if (m_path.size()>0 && !file_mapping::remove(m_path.c_str()))
		{
			std::cout<<"Failed to delete: \""<<m_path<<"\"";
		}
	}


#pragma warning ( disable : 4351 )
	yDecoder::yDecoder(Callbacks& callbacks):
		m_callbacks(callbacks),
		m_crcVal(-1L),
		m_crcAnz(0L),
		m_decodingState(ExpectingBegin),
		m_bytesDecoded(0),
		m_ybuf(),
		m_nbuf(0),
		m_binary_out(0)
	{}
#pragma warning ( default : 4351 )
	
	yDecoder::~yDecoder(){}

	inline void yDecoder::Flush()
	{
		if (m_nbuf == 0) return;
		yCacheFile::WriteToCacheFile(m_ybuf, m_nbuf, m_binary_out);
		m_nbuf = 0;
	}

	void yDecoder::Reset()
	{
		m_crcVal = -1L;
		m_crcAnz = 0L;
		m_decodingState = ExpectingBegin;
		m_bytesDecoded = 0;
		m_nbuf = 0;
	}

	inline void yDecoder::CrcAdd(int c)
	{
		unsigned long ch1, ch2, cc;

		/* X^32+X^26+X^23+X^22+X^16+X^12+X^11+X^10+X^8+X^7+X^5+X^4+X^2+X^1+X^0 */
		/* for (i = 0; i < size; i++) */
		/*      crccode = crc32Tab[(int) ((crccode) ^ (buf[i])) & 0xff] ^  */
		/*                (((crccode) >> 8) & 0x00FFFFFFL); */
		/*   return(crccode); */


		cc = (c)& 0x000000ffL;
		ch1 = (m_crcVal ^ cc) & 0xffL;
		ch1 = m_crcTab[ch1];
		ch2 = (m_crcVal >> 8L) & 0xffffffL;  // Correct version
		m_crcVal = ch1 ^ ch2;
		m_crcAnz++;
	}

	inline const char* yDecoder::findAttribute(const std::string& line, const char* name)
	{
		size_t pos = line.find(name);
		if (pos == std::string::npos) return 0;
		return &line[0] + pos + strlen(name);
	}
	
	void yDecoder::ReadLine(const char* msg, const std::size_t len)
	{
		switch (m_decodingState)
		{
			case ExpectingBegin:
				if (len>=7 && strncmp(msg,"=ybegin",7)==0)
				{
					const std::string line(msg,len);
					const char* p;
					p=findAttribute(line,"size=");
					if (0==p) 
					{
						m_callbacks.OnError("No size in =begin",FAILED);
						m_decodingState=ErrorState;
						return;
					}
					m_beginInfo.size = strtoull(p,0,10);
					p=findAttribute(line,"part=");
					m_beginInfo.multipart = (p!=0);
					m_beginInfo.part = m_beginInfo.multipart ? strtoul(p,0,10) : 0;
					p=findAttribute(line,"line=");
					m_beginInfo.line = strtoul(p,0,10);
					p=findAttribute(line,"name=");
					if (0==p) 
					{
						m_callbacks.OnError("No name in =begin",FAILED);
						m_decodingState=ErrorState;
						return;
					}
					m_beginInfo.name = p;
					if (!m_beginInfo.multipart)
					{
						m_binary_out = m_callbacks.OnBeginSegment(m_beginInfo);
						if (!m_binary_out)
						{
							m_callbacks.OnError("Error in =begin",FAILED);
							m_decodingState=ErrorState;
							return;
						}
						m_decodingState=ExpectingEnd;
					}
					else
					{
						m_decodingState=ExpectingPart;
					}
				}
				break;
			case ExpectingPart:
				if (len>=6 && strncmp(msg,"=ypart",6)==0)
				{
					const std::string line(msg,len);
					const char* p;
					p=findAttribute(line,"begin=");
					if (0==p) 
					{
						m_callbacks.OnError("No begin in =part",FAILED);
						m_decodingState=ErrorState;
						return;
					}
					m_partInfo.begin = strtoull(p,0,10);
					if (m_partInfo.begin<1 || m_partInfo.begin>m_beginInfo.size)
					{
						m_callbacks.OnError("begin is not valid in =part",FAILED);
						m_decodingState=ErrorState;
						return;	
					}
					p=findAttribute(line,"end=");
					if (0==p) 
					{
						m_callbacks.OnError("No end in =part",FAILED);
						m_decodingState=ErrorState;
						return;
					}
					m_partInfo.end = strtoull(p,0,10);
					if (m_partInfo.end<1 || m_partInfo.end>m_beginInfo.size)
					{
						std::ostringstream oss;
						oss<<"end="<<m_partInfo.end
							<<" is not valid in =part with size="<<m_beginInfo.size;
						m_callbacks.OnError(oss.str(),FAILED);
						m_decodingState=ErrorState;
						return;	
					}
					if (m_partInfo.end<m_partInfo.begin)
					{
						m_callbacks.OnError("end is less than begin =part",FAILED);
						m_decodingState=ErrorState;
						return;	
					}
					
					m_binary_out = m_callbacks.OnBeginSegment(m_beginInfo,m_partInfo);

					if (!m_binary_out)
					{
						m_callbacks.OnError("Error in =part",FAILED);
						m_decodingState=ErrorState;
						return;
					}
					
					m_decodingState=ExpectingEnd;
				}
				else
				{
					m_callbacks.OnError("Missing Part",FAILED);
					m_decodingState=ErrorState;
					return;
				}
				break;
			case ExpectingEnd:
				if (len>=5 && strncmp(msg,"=yend",5)==0)
				{
					const std::string line(msg,len);
					const char* p;
					p=findAttribute(line,"size=");
					if (0==p) 
					{
						Flush();
						m_callbacks.OnError("No size in =end",FAILED);
						m_decodingState=ErrorState;
						return;
					}
					m_endInfo.size = strtoull(p,0,10);
					if (m_beginInfo.multipart)
					{
						if (m_bytesDecoded!=m_partInfo.end-m_partInfo.begin+1)
						{
							Flush();
							m_callbacks.OnError("Mismatch decoded bytes do not match end-begin",SIZE_MISMATCH);
							m_decodingState=ErrorState;
							return;
						}
					}
					else
					{
						if (m_endInfo.size != m_beginInfo.size)
						{
							Flush();
							m_callbacks.OnError("Mismatch begin- and end size",SIZE_MISMATCH);
							m_decodingState=ErrorState;
							return;
						}
						if (m_bytesDecoded!=m_endInfo.size)
						{
							Flush();
							m_callbacks.OnError("Mismatch decoded bytes do not match size",SIZE_MISMATCH);
							m_decodingState=ErrorState;
							return;
						}
					}
					if (m_beginInfo.multipart)
					{
						p=findAttribute(line,"part=");
						if (0==p) 
						{
							Flush();
							m_callbacks.OnError("No part in =end",FAILED);
							m_decodingState=ErrorState;
							return;
						}
						m_endInfo.part = strtoul(p,0,10);
						if (m_endInfo.part != m_beginInfo.part)
						{
							Flush();
							m_callbacks.OnError("Mismatch part",PART_MISMATCH);
							m_decodingState=ErrorState;
							return;
						}
					}
					p=findAttribute(line,"pcrc32=");
					if (p==0) p=findAttribute(line,"crc32=");
					if (0==p) 
					{
						Flush();
						m_callbacks.OnError("No crc in =end",FAILED);
						m_decodingState=ErrorState;
						return;
					}
					m_endInfo.crc = strtoul(p,0,16);
					if (m_endInfo.crc != ((m_crcVal^0xFFFFFFFFl)&0xFFFFFFFFl))
					{
						m_callbacks.OnWarning("Crc mismatch",PART_CRC_MISMATCH);
					}
					Flush();
					m_callbacks.OnEndSegment(m_endInfo);
					m_decodingState=ExpectingBegin;
				}
				else
				{
					for(size_t i=0;i<len;i++)
					{
						unsigned char c=msg[i];
						switch (c)
						{
						case '\r': continue;
						case '\n': continue;
						case '=':
						{
							if (++i == len)
							{
								m_callbacks.OnError("Escape was last char in line", FAILED);
								m_decodingState = ErrorState;
								return;
							}
							c = msg[i];
							c -= 64;
						}
							// Intentionally fall through:
						default:
							c -= 42;
							CrcAdd(c);
							++m_bytesDecoded;
							m_ybuf[m_nbuf++] = c;
							if (m_nbuf == YBUFSIZE) Flush();
						}
					}
				}
			break;
			case ErrorState: 
			break;
		}		
	}
	
	void yDecoder::Error(const std::string& msg, const Status& status)
	{
		m_callbacks.OnError(msg,status);
		Reset();
	}
	
	void yDecoder::Warning(const std::string& msg, const Status& status)
	{
		m_callbacks.OnWarning(msg,status);
	}

/*static*/ uint32_t yDecoder::m_crcTab[256] =
	{
		0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
		0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
		0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
		0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
		0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
		0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
		0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
		0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
		0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
		0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
		0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
		0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
		0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
		0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
		0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
		0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
		0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
		0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
		0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
		0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
		0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
		0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
		0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
		0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
		0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
		0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
		0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
		0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
		0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
		0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
		0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
		0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
	};

}

