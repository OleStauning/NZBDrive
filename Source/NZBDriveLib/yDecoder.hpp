/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef YDECODER_HPP
#define YDECODER_HPP

#include <iostream>
#include <fstream>
#include <string>

#include <filesystem>

namespace ByteFountain
{

class yDecoder
{
public:

	static const std::size_t YBUFSIZE = 1024;
	
	enum Status
	{
		SUCCESS = 0, /**< The decoding succeeded */
		CRC_MISMATCH = 1, /**< The crc value of the decoded data doesn't match the crc value in the trailer */
		PART_CRC_MISMATCH = 2, /**< The crc value of the decoded part data doesn't match the part crc value in the trailer */
		PART_MISMATCH = 4, /**< The part number in the trailer doesn't match the part number in the header */
		SIZE_MISMATCH = 8, /**< The size of the data or the size value in the trailer doesn't match the size value in the header */
		NAME_MISMATCH = 16, /**< The name value in the header doesn't match the name of the previous parts */
		FAILED =  32/**< The decoding failed */
	};

	struct yBeginInfo
	{
		yBeginInfo():multipart(false),part(0),line(0),size(0),name(){}
		bool multipart;
		unsigned long part;
		unsigned long line;
		unsigned long long size;
		std::filesystem::path name;
	};

	struct yPartInfo
	{
		yPartInfo():begin(0),end(0){}
		unsigned long long begin;
		unsigned long long end;
	};

	struct yEndInfo
	{
		yEndInfo():size(0),part(0),crc(0){}
		unsigned long long size;
		unsigned long part;
		unsigned long crc;
	};

	class Callbacks
	{
	public:
		/// Called when a new segment starts:
		// the method should return an ostream that can be used to write the segment data. 
		virtual void OnBeginSegment(const yBeginInfo& beginInfo, const yPartInfo& partInfo)=0;
		virtual void OnBeginSegment(const yBeginInfo& beginInfo)=0;
		virtual void OnData(const unsigned char* buf, const std::size_t size)=0;
		virtual void OnEndSegment(const yEndInfo& endInfo)=0;
		virtual void OnError(const std::string& msg, const Status& status)=0;
		virtual void OnWarning(const std::string& msg, const Status& status)=0;
		virtual ~Callbacks()=0;
	};

	yDecoder(Callbacks& callbacks);
	virtual ~yDecoder();

	void ReadLine(const char* line, const std::size_t len);
	void Reset();
	
private:
	

	enum DecodingState
	{
		ExpectingBegin,
		ExpectingPart,
		ExpectingEnd,
		ErrorState
	};
	
	static uint32_t m_crcTab[256];
	
	Callbacks& m_callbacks;
	unsigned long m_crcVal;
	unsigned long m_crcAnz;
	DecodingState m_decodingState;
	yBeginInfo m_beginInfo;
	yPartInfo m_partInfo;
	yEndInfo m_endInfo;
	unsigned long long m_bytesDecoded;
	unsigned char m_ybuf[YBUFSIZE];
	std::string m_lineBuffer;
	std::size_t m_nbuf;
	
	void CrcAdd(int c);
	const char* findAttribute(const std::string& line, const char* name);
	void Error(const std::string& msg, const Status& status);	
	void Warning(const std::string& msg, const Status& status);
	void Flush();
	
};

}
#endif
