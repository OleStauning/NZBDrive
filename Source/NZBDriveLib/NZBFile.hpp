/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef NZBFILE_HPP
#define NZBFILE_HPP

#include "InternalFile.hpp"
#include "InternalFileVisitor.hpp"
#include "SegmentState.hpp"
#include "yDecoder.hpp"
//#include "client_cache.hpp"
#include "NewsClientCache.hpp"
#include "IArticleStreamHandler.hpp"
#include "Logger.hpp"
#include "nzb.hpp"
#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <boost/asio.hpp>
#include <filesystem>
#include <optional>
#include "SegmentCache.hpp"

namespace ByteFountain
{

class NZBDirectory;

class NZBFile : public InternalFile, public std::enable_shared_from_this<NZBFile>
{
public:
	typedef std::function< void(const std::filesystem::path& filename) > OnFilenameFunction;
	typedef std::function< void(const int32_t nzbID, const int32_t fileID, const int32_t segments, const uint64_t size) > FileAddedFunction;
	typedef std::function< void(const int32_t fileID, const std::filesystem::path& name, const uint64_t size) > FileInfoFunction;
	typedef std::function< void(const int32_t fileID, const int32_t segment, const SegmentState state) > FileSegmentStateChangedFunction;
	typedef std::function< void(const int32_t fileID) > FileRemovedFunction;

private:

	typedef yDecoder::yBeginInfo yBeginInfo;
	typedef yDecoder::yPartInfo yPartInfo;
	typedef yDecoder::yEndInfo yEndInfo;
	typedef yDecoder::Status Status;

	struct SegmentInfo
	{
		SegmentState status;
		std::vector<std::string> msgIDs;
		unsigned long msgBytes;
		std::optional<yPartInfo> ypart;
		unsigned long long end_boundary;

		static const char* Name(const SegmentState status)
		{
			switch(status)
			{
			case SegmentState::None: return "none";
			case SegmentState::Loading: return "loading";
			case SegmentState::HasData: return "has data";
			case SegmentState::MissingSegment: return "missing segment";
			case SegmentState::DownloadFailed: return "download failed";
			}
			return "unknown";
		}

		SegmentInfo();
	};

	boost::asio::io_service& m_io_service;
	const int32_t m_nzbID;
	const int32_t m_fileID;
	NewsClientCache& m_clients;
	SegmentCache& m_segment_cache;
	std::vector<std::string> m_groups;
	std::vector<SegmentInfo> m_segments;

	unsigned long m_checked_segments;
	unsigned long m_missing_segments;
	unsigned long m_requested_segments;
	std::optional<yBeginInfo> m_beginInfo;
	
	std::condition_variable m_cond;
	std::mutex m_mutex;

	Logger& m_log;

	FileAddedFunction m_fileAddedFunction;
	FileInfoFunction m_fileInfoFunction;
	FileSegmentStateChangedFunction m_fileSegmentStateChangedFunction;
	FileRemovedFunction m_fileRemovedFunction;

	std::atomic<int> m_decoders;

	bool SetBeginInfo(const yBeginInfo& beginInfo);

	void UpdateVBegin();
	
	std::tuple<std::size_t, std::size_t> FindSegmentIdxRange(const unsigned long long begin, const unsigned long long end);
	
	std::tuple<bool, std::size_t> TryGetData(std::unordered_set<std::size_t>& done, 
		char* buf, const unsigned long long offset, const std::size_t size, bool& err,
		CancelSignal* cancel=0, const bool priority=false);

	std::atomic<uint_fast64_t> m_id_counter;
	
	struct AsyncDataRequest
	{
		uint_fast64_t id;
		OnDataFunction func;
		char* buf;
		unsigned long long offset;
		std::size_t size;
		boost::signals2::scoped_connection conn;
		bool priority;
		std::unordered_set<std::size_t> done;
		AsyncDataRequest(uint_fast64_t i, OnDataFunction f, char* b, const unsigned long long o, const std::size_t s,
			boost::signals2::connection c, const bool prio, const std::unordered_set<std::size_t>& d) :
			id(i), func(f), buf(b), offset(o), size(s), conn(c), priority(prio), done(d)
		{}
	};

	std::list< AsyncDataRequest > m_data_requests;

	void ProcessAsyncDataRequests();
	
	struct AsyncFilenameRequest
	{
		uint_fast64_t id;
		OnFilenameFunction func;
		boost::signals2::scoped_connection conn;
		AsyncFilenameRequest(uint_fast64_t i, OnFilenameFunction f, boost::signals2::connection c):id(i),func(f),conn(c){}
		~AsyncFilenameRequest(){}
	};

	std::list< AsyncFilenameRequest > m_filename_requests;
public:

//	OnFilenameHandle OnFilename;

	NZBFile(boost::asio::io_service& io_service, const int32_t nzbID, const int32_t fileID,
		const nzb::file& nzbfile, NewsClientCache& clients, SegmentCache& segment_cache, Logger& log,
		NZBFile::FileAddedFunction fileAddedFunction = nullptr,
		NZBFile::FileInfoFunction fileInfoFunction = nullptr,
		NZBFile::FileSegmentStateChangedFunction fileSegmentStateChangedFunction = nullptr,
		NZBFile::FileRemovedFunction fileRemovedFunction = nullptr
		);

	virtual ~NZBFile();

	void SetFileInfo(const std::size_t idx, const yBeginInfo& beginInfo, const yPartInfo& partInfo);
	void SetFileInfo(const yBeginInfo& beginInfo);

	void SetSegmentFinished(const std::size_t idx);
	void SetDownloadFailed(const std::size_t idx, const FileErrorFlag reason);

	friend struct MyDecoder;

	struct MyDecoder : public yDecoder::Callbacks
	{
		enum State { Init, Begin, End, Error, Canceled } m_state;
		std::shared_ptr<NZBFile> m_file;
		yDecoder m_decoder;
		const std::size_t m_idx;
		std::shared_ptr<CachedSegment> m_cached_segment;
		std::size_t m_offset;

		MyDecoder(std::shared_ptr<NZBFile> file, const std::size_t idx);
		virtual ~MyDecoder();

		void Decode(const char* yenc_data, const std::size_t len);
		void Reset();

		void OnBeginSegment(const yBeginInfo& beginInfo, const yPartInfo& partInfo);
		void OnBeginSegment(const yBeginInfo& beginInfo);
		void OnData(const unsigned char* buf, const std::size_t size);
		void OnEndSegment(const yEndInfo& endInfo);
		void OnError(const std::string& msg, const Status& status);
		void OnWarning(const std::string& msg, const Status& status);
	};
	struct MyArticleDecoder : public IArticleStreamHandler
	{
		IFile::ConCancelScoped m_cancel;
		std::shared_ptr<NZBFile> m_file;
		MyDecoder m_decoder;

		MyArticleDecoder(std::shared_ptr<NZBFile> file, const std::size_t idx, IFile::CancelSignal* cancel);
		virtual ~MyArticleDecoder();
		void OnErrorRetry();
		void OnCancel();
		void OnErrorCancel();
		void OnData(const char* msg, const std::size_t len);
		void OnEnd();
	};

	void Decode(std::shared_ptr<MyDecoder> decoder, const bool code, const std::string& article_data);
	void OnStatResponse(const unsigned long idx, const bool have);
	bool Preload(const bool pre_check_segments, CancelSignal* cancel=0);
	bool ReadSegment(const std::size_t idx, CancelSignal* cancel = 0, const bool priority = false);
//	bool AssertBeginInfo();
	void RemoveCacheFile();

	std::filesystem::path GetFileName();
	unsigned long long GetFileSize();
	bool GetFileData(char* buf, const unsigned long long offset, const std::size_t size, std::size_t& readsize);

	void CancelAsyncGetFileData(const uint_fast64_t& id);
	void AsyncGetFileData(OnDataFunction func, char* buf, const unsigned long long offset, const std::size_t size,
		CancelSignal* cancel = 0, const bool priority = false);

	void CancelAsyncGetFilename(const uint_fast64_t& id);
	void AsyncGetFilename(OnFilenameFunction func, CancelSignal* cancel=0);

	unsigned long long CountMissingBytesInRange(const unsigned long long begin, const unsigned long long end);
	bool BufferNextSegmentInRange(const unsigned long long begin, const unsigned long long end);
	
	void Accept(InternalFileVisitor& visitor) const { visitor.Visit(*this); }

};

}

#endif
