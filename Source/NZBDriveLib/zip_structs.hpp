/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef ZIPSTRUCTS
#define ZIPSTRUCTS
#include <iostream>


namespace zip
{
	

#pragma pack(1)

	struct local_file_header
	{
//		local file header signature     4 bytes  (0x04034b50)
		unsigned int local_file_header_signature;
//		version needed to extract       2 bytes
		unsigned short int extract_method;
//		general purpose bit flag        2 bytes
		unsigned short int general_purpose_flags;
//		compression method              2 bytes
		unsigned short int compression_method;
//		last mod file time              2 bytes
		unsigned short int last_mod_file_time;
//		last mod file date              2 bytes
		unsigned short int last_mod_file_date;
//		crc-32                          4 bytes
		unsigned int crc_32;
//		compressed size                 4 bytes
		unsigned int compressed_size;
//		uncompressed size               4 bytes
		unsigned int uncompressed_size;
//		file name length                2 bytes
		unsigned short int file_name_length;
//		extra field length              2 bytes
		unsigned short int extra_field_length;

//		file name (variable size)
//		extra field (variable size)
		
	}
#ifndef _MSC_VER
	__attribute__((__packed__))
#endif
	;
	
	struct data_descriptor
	{
//		crc-32                          4 bytes
		unsigned int crc_32;
//		compressed size                 4 bytes
		unsigned int compressed_size;
//		uncompressed size               4 bytes
		unsigned int uncompressed_size;
	}
#ifndef _MSC_VER
	__attribute__((__packed__))
#endif
	;
	
#pragma pack()
	
	
	inline bool Validate(const local_file_header& lfh)
	{ 
		return lfh.local_file_header_signature==0x04034b50; 
	}

	
	inline std::ostream& print(std::ostream& ost, const local_file_header& lfh)
	{
		ost<<"LOCAL FILE HEADER:"<<std::endl;
		ost<<"local_file_header_signature:"<<std::hex<<(int)lfh.local_file_header_signature<<std::endl;
		ost<<"extract_method:"<<std::hex<<(int)lfh.extract_method<<std::endl;
		ost<<"general_purpose_flags:"<<std::hex<<(int)lfh.general_purpose_flags<<std::endl;
		ost<<"compression_method:"<<std::hex<<(int)lfh.compression_method<<std::endl;
		ost<<"last_mod_file_time:"<<std::hex<<(int)lfh.last_mod_file_time<<std::endl;
		ost<<"last_mod_file_date:"<<std::hex<<(int)lfh.last_mod_file_date<<std::endl;
		ost<<"crc_32:"<<std::hex<<(int)lfh.crc_32<<std::endl;
		ost<<"compressed_size:"<<std::hex<<(int)lfh.compressed_size<<std::endl;
		ost<<"uncompressed_size:"<<std::hex<<(int)lfh.uncompressed_size<<std::endl;
		ost<<"file_name_length:"<<std::hex<<(int)lfh.file_name_length<<std::endl;
		ost<<"extra_field_length:"<<std::hex<<(int)lfh.extra_field_length<<std::endl;
		ost<<std::dec;
		return ost;
	}


	

	
}

#endif
