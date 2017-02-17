/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef RARSTRUCTS
#define RARSTRUCTS
#include <iostream>

namespace rar
{
	
	enum HeaderType { Mark=0x72, Main=0x73, File=0x74, SubBlock=0x7a };

	#pragma pack(1)

	struct mark_head
	{
	//	   Marker block ( MARK_HEAD )
	// HEAD_CRC        Always 0x6152
	// 2 bytes

		unsigned short int head_crc;

	// HEAD_TYPE       Header type: 0x72
	// 1 byte

		unsigned char header_type;

	// HEAD_FLAGS      Always 0x1a21
	// 2 bytes

		unsigned short int head_flags;

	// HEAD_SIZE       Block size = 0x0007
	// 2 bytes

		unsigned short int head_size;

	//    The marker block is actually considered as a fixed byte
	// sequence: 0x52 0x61 0x72 0x21 0x1a 0x07 0x00	
	}
#ifndef WIN32
	__attribute__((__packed__))
#endif
	;

	inline bool Validate(const mark_head& head)
	{ 
		return head.head_crc==0x6152 && head.header_type==0x72 && head.head_flags==0x1a21 && head.head_size==0x0007; 
	}
	
	
	struct main_head
	{
	//   Archive header ( MAIN_HEAD )

	// HEAD_CRC        CRC of fields HEAD_TYPE to RESERVED2
	// 2 bytes

		unsigned short int head_crc;

	// HEAD_TYPE       Header type: 0x73
	// 1 byte

		unsigned char head_type;

	// HEAD_FLAGS      Bit flags:
	// 2 bytes
	/*              0x0001  - Volume attribute (archive volume)
			0x0002  - Archive comment present
				RAR 3.x uses the separate comment block
				and does not set this flag.

			0x0004  - Archive lock attribute
			0x0008  - Solid attribute (solid archive)
			0x0010  - New volume naming scheme (\'volname.partN.rar\')
			0x0020  - Authenticity information present
				RAR 3.x does not set this flag.

			0x0040  - Recovery record present
			0x0080  - Block headers are encrypted
			0x0100  - First volume (set only by RAR 3.0 and later)

			other bits in HEAD_FLAGS are reserved for
			internal use
	*/

		unsigned short int head_flags;

	// HEAD_SIZE       Archive header total size including archive comments
	// 2 bytes

		unsigned short int head_size;

	// RESERVED1       Reserved
	// 2 bytes

		unsigned short int reserved1;

	// RESERVED2       Reserved
	// 4 bytes

		unsigned int reserved2;
	}
#ifndef WIN32
	__attribute__((__packed__))
#endif
	;

	inline bool Validate(const main_head& head)
	{ 
		return head.head_type==0x73;
	}
	
	
	struct rar_head
	{
		mark_head mark;
		main_head main;
	}
#ifndef WIN32
	__attribute__((__packed__))
#endif
	;

	inline bool Validate(const rar_head& head)
	{ 
		return Validate(head.mark) && Validate(head.main);
	}
		
	struct file_head
	{

	// File header (File in archive)

	// HEAD_CRC        CRC of fields from HEAD_TYPE to FILEATTR
	// 2 bytes         and file name

		unsigned short int head_crc;

	// HEAD_TYPE       Header type: 0x74
	// 1 byte

		unsigned char head_type;

	// HEAD_FLAGS      Bit flags:
	// 2 bytes
	//                 0x01 - file continued from previous volume
	//                 0x02 - file continued in next volume
	//                 0x04 - file encrypted with password
	// 
	//                 0x08 - file comment present
	//                        RAR 3.x uses the separate comment block
	//                        and does not set this flag.
	// 
	//                 0x10 - information from previous files is used (solid flag)
	//                        (for RAR 2.0 and later)
	// 
	//                 bits 7 6 5 (for RAR 2.0 and later)
	// 
	//                      0 0 0    - dictionary size   64 KB
	//                      0 0 1    - dictionary size  128 KB
	//                      0 1 0    - dictionary size  256 KB
	//                      0 1 1    - dictionary size  512 KB
	//                      1 0 0    - dictionary size 1024 KB
	//                      1 0 1    - dictionary size 2048 KB
	//                      1 1 0    - dictionary size 4096 KB
	//                      1 1 1    - file is directory
	// 
	//                0x100 - HIGH_PACK_SIZE and HIGH_UNP_SIZE fields
	//                        are present. These fields are used to archive
	//                        only very large files (larger than 2Gb),
	//                        for smaller files these fields are absent.
	// 
	//                0x200 - FILE_NAME contains both usual and encoded
	//                        Unicode name separated by zero. In this case
	//                        NAME_SIZE field is equal to the length
	//                        of usual name plus encoded Unicode name plus 1.
	// 
	//                0x400 - the header contains additional 8 bytes
	//                        after the file name, which are required to
	//                        increase encryption security (so called 'salt').
	// 
	//                0x800 - Version flag. It is an old file version,
	//                        a version number is appended to file name as ';n'.
	// 
	//               0x1000 - Extended time field present.
	// 
	//               0x8000 - this bit always is set, so the complete
	//                        block size is HEAD_SIZE + PACK_SIZE
	//                        (and plus HIGH_PACK_SIZE, if bit 0x100 is set)

		unsigned short int head_flags;

	// HEAD_SIZE       File header full size including file name and comments
	// 2 bytes

		unsigned short int head_size;

	// PACK_SIZE       Compressed file size
	// 4 bytes

		unsigned int pack_size;

	// UNP_SIZE        Uncompressed file size
	// 4 bytes

		unsigned int unp_size;

	// HOST_OS         Operating system used for archiving
	// 1 byte                0 - MS DOS
	//                       1 - OS/2
	//                       2 - Win32
	//                       3 - Unix
	//                       4 - Mac OS
	//                       5 - BeOS

		unsigned char host_os;

	// FILE_CRC        File CRC
	// 4 bytes

		unsigned int file_crc;

	// FTIME           Date and time in standard MS DOS format
	// 4 bytes

		unsigned int ftime;

	// UNP_VER         RAR version needed to extract file
	// 1 byte
	//                Version number is encoded as
	//                10 * Major version + minor version.

		unsigned char unp_ver;

	// METHOD          Packing method
	// 1 byte
	//                0x30 - storing
	//                0x31 - fastest compression
	//                0x32 - fast compression
	//                0x33 - normal compression
	//                0x34 - good compression
	//                0x35 - best compression

		unsigned char method;

	// NAME_SIZE       File name size
	// 2 bytes

		unsigned short int name_size;

	// ATTR            File attributes
	// 4 bytes

		unsigned int attr;

	}
#ifndef WIN32
	__attribute__((__packed__))
#endif
	;
	
	inline bool Validate(const file_head& head, const unsigned char header_type)
	{ 
		return head.head_type==header_type && head.host_os<=5 && 0x30<=head.method && head.method<=0x35 && head.name_size<300;
	}

	struct high_size
	{
		unsigned int high_pack_size;
		unsigned int high_unpack_size;
	}
#ifndef WIN32
	__attribute__((__packed__))
#endif
	;
	
	struct sub_block
	{
	// 2 bytes

		unsigned short int head_crc;

	// HEAD_TYPE       Header type: 0x7a
	// 1 byte

		unsigned char header_type;

		unsigned short int head_flags;

		unsigned short int head_size;

		unsigned int data_size;

		unsigned short int sub_type;
		
		unsigned char reserved1;
		
	}
#ifndef WIN32
	__attribute__((__packed__))
#endif
	;
	
	inline bool Validate(const sub_block& block)
	{ 
		return block.header_type==0x7a;
	}
	
#pragma pack()
	

	inline std::ostream& print(std::ostream& ost, const mark_head& mark)
	{
		ost<<"MARK_HEAD:"<<std::endl;
		ost<<"head_crc:"<<std::hex<<(int)mark.head_crc<<std::endl;
		ost<<"header_type:"<<std::hex<<(int)mark.header_type<<std::endl;
		ost<<"head_flags:"<<std::hex<<(int)mark.head_flags<<std::endl;
		ost<<"head_size:"<<std::hex<<(int)mark.head_size<<std::endl;
		ost<<std::dec;
		return ost;
	}

	inline std::ostream& print(std::ostream& ost, const main_head& main)
	{
		ost<<"MAIN_HEAD:"<<std::endl;
		ost<<"head_crc:"<<std::hex<<(int)main.head_crc<<std::endl;
		ost<<"head_type:"<<std::hex<<(int)main.head_type<<std::endl;
		ost<<"head_flags:"<<std::hex<<(int)main.head_flags<<std::endl;
		if (0!=(main.head_flags&0x0001)) ost<<"  - Volume attribute (archive volume)"<<std::endl;
		if (0!=(main.head_flags&0x0002)) ost<<"  - Archive comment present"<<std::endl;
		if (0!=(main.head_flags&0x0004)) ost<<"  - Archive lock attribute"<<std::endl;
		if (0!=(main.head_flags&0x0008)) ost<<"  - Solid attribute (solid archive)"<<std::endl;
		if (0!=(main.head_flags&0x0010)) ost<<"  - New volume naming scheme (volname.partN.rar)"<<std::endl;
		if (0!=(main.head_flags&0x0020)) ost<<"  - Authenticity information present"<<std::endl;
		if (0!=(main.head_flags&0x0040)) ost<<"  - Recovery record present"<<std::endl;
		if (0!=(main.head_flags&0x0080)) ost<<"  - Block headers are encrypted"<<std::endl;
		if (0!=(main.head_flags&0x0100)) ost<<"  - First volume (set only by RAR 3.0 and later)"<<std::endl;
		ost<<"head_size:"<<std::hex<<(int)main.head_size<<std::endl;
		ost<<std::dec;
		return ost;
	}

	inline std::ostream& print(std::ostream& ost, const file_head& file)
	{
		ost<<"FILE_HEAD:"<<std::endl;
		ost<<"head_crc:"<<std::hex<<(int)file.head_crc<<std::endl;
		ost<<"head_type:"<<std::hex<<(int)file.head_type<<std::endl;
		ost<<"head_flags:"<<std::hex<<(int)file.head_flags<<std::endl;
		if (0!=(file.head_flags&0x01)) ost<<"  - file continued from previous volume"<<std::endl;
		if (0!=(file.head_flags&0x02)) ost<<"  - file continued in next volume"<<std::endl;
		if (0!=(file.head_flags&0x04)) ost<<"  - file encrypted with password"<<std::endl;
		if (0!=(file.head_flags&0x08)) ost<<"  - file comment present"<<std::endl;
		if (0!=(file.head_flags&0x10)) ost<<"  - information from previous files is used (solid flag)"<<std::endl;
		ost<<"head_size:"<<std::hex<<(int)file.head_size<<std::endl;
		ost<<"pack_size:"<<std::hex<<(int)file.pack_size<<std::endl;
		ost<<"unp_size:"<<std::hex<<(int)file.unp_size<<std::endl;
		ost<<"host_os:"<<std::hex<<(int)file.host_os<<std::endl;
		ost<<"file_crc:"<<std::hex<<(int)file.file_crc<<std::endl;
		ost<<"ftime:"<<std::hex<<(int)file.ftime<<std::endl;
		ost<<"unp_ver:"<<std::hex<<(int)file.unp_ver<<std::endl;
		ost<<"method:"<<std::hex<<(int)file.method<<std::endl;
		ost<<"name_size:"<<std::hex<<(int)file.name_size<<std::endl;
		ost<<"attr:"<<std::hex<<(int)file.attr<<std::endl;
		ost<<std::dec;
		return ost;
	}

	inline std::ostream& print(std::ostream& ost, const rar_head& rarhead)
	{
		print(ost,rarhead.mark);
		print(ost,rarhead.main);
		return ost;
	}

	inline std::ostream& print(std::ostream& ost, const sub_block& sblock)
	{
		ost<<"FILE_HEAD:"<<std::endl;
		ost<<"head_crc:"<<std::hex<<(int)sblock.head_crc<<std::endl;
		ost<<"head_type:"<<std::hex<<(int)sblock.header_type<<std::endl;
		ost<<"head_flags:"<<std::hex<<(int)sblock.head_flags<<std::endl;
		ost<<"head_size:"<<std::hex<<(int)sblock.head_size<<std::endl;
		ost<<"data_size:"<<std::hex<<(int)sblock.data_size<<std::endl;
		ost<<"sub_type:"<<std::hex<<(int)sblock.sub_type<<std::endl;
		ost<<std::dec;
		return ost;
	}
	

	
}
#endif