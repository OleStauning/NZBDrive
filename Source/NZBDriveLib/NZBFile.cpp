/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#include "NZBFile.hpp"
#include "yDecoder.hpp"
#include "text_tool.hpp"
#include <algorithm>
#include <functional>
#include <memory>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>


namespace ByteFountain
{

template <typename T, typename... TArgs>
static void SendEvent(T& handler, TArgs&&... args)
{
	auto h = handler;
	if (h) h(std::forward<TArgs>(args)...);
}

NZBFile::SegmentInfo::SegmentInfo():
	status(SegmentState::None), msgIDs(), msgBytes(0), ypart(std::nullopt), end_boundary(0)
{}
	
NZBFile::NZBFile(boost::asio::io_service& io_service, const int32_t nzbID, const int32_t fileID, 
	const nzb::file& nzbfile, NewsClientCache& clients, SegmentCache& segment_cache, Logger& log,
	NZBFile::FileAddedFunction fileAddedFunction,
	NZBFile::FileInfoFunction fileInfoFunction,
	NZBFile::FileSegmentStateChangedFunction fileSegmentStateChangedFunction,
	NZBFile::FileRemovedFunction fileRemovedFunction
	) :
	m_io_service(io_service),
	m_nzbID(nzbID),
	m_fileID(fileID),
	m_clients(clients),
	m_segment_cache(segment_cache),
	m_groups(nzbfile.groups.begin(),nzbfile.groups.end()),
	m_checked_segments(0),
	m_missing_segments(0),
	m_requested_segments(0),
	m_beginInfo(std::nullopt),
	m_log(log),
	m_fileAddedFunction(fileAddedFunction),
	m_fileInfoFunction(fileInfoFunction),
	m_fileSegmentStateChangedFunction(fileSegmentStateChangedFunction),
	m_fileRemovedFunction(fileRemovedFunction),
	m_decoders(0),
	m_id_counter(0)
{
	m_segments.reserve(nzbfile.segments.size());
	
	for(const nzb::file::segment& s : nzbfile.segments)
	{
		if (s.number<1 || s.number>nzbfile.segments.size())
		{
			m_log<<Logger::Error<<"Invalid segment number "<<s.number<<" in NZB file with subject :"<<
				text_tool::unicode_to_utf8(nzbfile.subject)<<Logger::End;
			SetErrorFlag(FileErrorFlag::InvalidSegmentNumber);
//			continue;
		}
		if(s.bytes<0)
		{
			m_log<<Logger::Error<<"Invalid segment size "<<s.bytes<<" in NZB file with subject :"<<
				text_tool::unicode_to_utf8(nzbfile.subject)<<Logger::End;
			SetErrorFlag(FileErrorFlag::InvalidSegmentSize);
			continue;
		}
		if (s.message_id.size()==0)
		{
			m_log<<Logger::Error<<"Empty segment message ID in NZB file with subject :"<<
				text_tool::unicode_to_utf8(nzbfile.subject)<<Logger::End;
			SetErrorFlag(FileErrorFlag::EmptyMessageID);
			continue;
		}

		if (m_segments.size()<s.number) m_segments.resize(s.number);
		
		m_segments[s.number-1].msgIDs.push_back(s.message_id);
		m_segments[s.number-1].msgBytes=s.bytes; // TODO: Maybe redundant segments have different sizes??
	}

	uint64_t bytes = 0;

	for (std::size_t i = 0; i<m_segments.size(); ++i)
	{
		SegmentInfo& s(m_segments[i]);
		if (s.msgIDs.empty())
		{
			m_log<<Logger::Error<<"Missing segment number "<<i+1<<" in NZB file with subject :"
				<<text_tool::unicode_to_utf8(nzbfile.subject)<<Logger::End;
			SetErrorFlag(FileErrorFlag::MissingSegment);
		}

		bytes += s.msgBytes;
	}

	SendEvent(m_fileAddedFunction, m_nzbID, m_fileID, (int32_t)m_segments.size(), bytes);

	UpdateVBegin();
}

NZBFile::~NZBFile()
{
	SendEvent(m_fileRemovedFunction, m_fileID);
	
	assert(m_decoders==0);
}


bool NZBFile::SetBeginInfo(const yBeginInfo& beginInfo)	
{
	if (!m_beginInfo)
	{
		m_beginInfo=beginInfo;

		SendEvent(m_fileInfoFunction, m_fileID, m_beginInfo->name, m_beginInfo->size);

		UpdateVBegin();

		return true;
	}
	
	return false;
}

void NZBFile::SetFileInfo(const std::size_t idx, const yBeginInfo& beginInfo, const yPartInfo& partInfo)
{
	bool newfile=false;
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		newfile=SetBeginInfo(beginInfo);

		m_segments[idx].ypart = partInfo;

		m_cond.notify_all();
	}
	if (newfile)
	{
		for(const auto& req : m_filename_requests) 
		{
			m_io_service.post( [func=req.func, name=m_beginInfo->name](){ func(name); } );
		}
		m_filename_requests.clear();
	}
}

void NZBFile::SetFileInfo(const yBeginInfo& beginInfo)
{
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		SetBeginInfo(beginInfo);
		
		yPartInfo partInfo;
		partInfo.begin = 1;
		partInfo.end = beginInfo.size+1;
		
		m_segments[0].ypart = partInfo;

		m_cond.notify_all();
	}
	for(const auto& req : m_filename_requests)
	{
			m_io_service.post( [func=req.func, name=m_beginInfo->name](){ func(name); } );
	}
	m_filename_requests.clear();
}

void NZBFile::SetSegmentFinished(const std::size_t idx)
{
	SendEvent(m_fileSegmentStateChangedFunction, m_fileID, (int32_t)idx, SegmentState::HasData);

	{
		std::unique_lock<std::mutex> lock(m_mutex);

		m_segments[idx].status = SegmentState::HasData;

		UpdateVBegin();
	}
	ProcessAsyncDataRequests();

	m_cond.notify_all();
}

void NZBFile::SetDownloadFailed(const std::size_t idx, const FileErrorFlag reason)
{
	SetErrorFlag(reason);

	SendEvent(m_fileSegmentStateChangedFunction, m_fileID, (int32_t)idx, SegmentState::DownloadFailed);

	{
		std::unique_lock<std::mutex> lock(m_mutex);

		m_segments[idx].status = SegmentState::DownloadFailed;

		UpdateVBegin();
	}

	ProcessAsyncDataRequests();

	m_cond.notify_all();

	if (!m_beginInfo)
	{
		for(const auto& req : m_filename_requests) 
		{
			m_io_service.post( [func=req.func](){ func(""); } );
		}
		m_filename_requests.clear();
	}
}

NZBFile::MyDecoder::MyDecoder(std::shared_ptr<NZBFile> file, const std::size_t idx) :
	m_state(NZBFile::MyDecoder::Init),
	m_file(file),
	m_decoder(*this),
	m_idx(idx),
	m_cached_segment(),
	m_offset(0)
{
}
NZBFile::MyDecoder::~MyDecoder()
{
	m_cached_segment.reset();
}

void NZBFile::MyDecoder::Decode(const char* yenc_data, const std::size_t len) { m_decoder.ReadLine(yenc_data, len); }
void NZBFile::MyDecoder::Reset() { m_decoder.Reset(); }

void NZBFile::MyDecoder::OnBeginSegment(const yBeginInfo& beginInfo, const yPartInfo& partInfo)
{
	m_state=NZBFile::MyDecoder::Begin;
	m_file->SetFileInfo(m_idx,beginInfo,partInfo);

	if (m_file->m_beginInfo->size != beginInfo.size)
	{
		m_file->m_log<<Logger::Error<<"=ybegin size parameter mismatch when reading the parts of: \"" << m_file->m_beginInfo->name << "\"" << Logger::End;
	}
	if (m_file->m_beginInfo->size < partInfo.end)
	{
		m_file->m_log<<Logger::Error<<"=ybegin size parameter less than =ypart end parameter: \"" << m_file->m_beginInfo->name << Logger::End;
	}
	else
	{
		m_file->m_log<<Logger::Info<<"BeginSegment: "<<m_idx<<" "<<partInfo.begin-1<<"-"<<partInfo.end-1<<" "<<m_file->m_beginInfo->name<<Logger::End;

		if (m_state == NZBFile::MyDecoder::Canceled) return;

		auto size = (std::size_t)(partInfo.end - partInfo.begin + 1);
		m_cached_segment = m_file->m_segment_cache.Create(m_file->m_fileID, m_idx, size);
	}
}
void NZBFile::MyDecoder::OnBeginSegment(const yBeginInfo& beginInfo)
{
	m_state=NZBFile::MyDecoder::Begin;
	m_file->SetFileInfo(beginInfo);

	if (m_file->m_beginInfo->size != beginInfo.size)
	{
		m_file->m_log<<Logger::Error<<"=ybegin size parameter mismatch when reading the parts of: \"" << m_file->m_beginInfo->name << "\"" << Logger::End;
	}
		
	m_file->m_log<<Logger::Debug<<"BeginSegment: "<<m_idx<<" 0-"<<beginInfo.size-1<<" "<<m_file->m_beginInfo->name<<Logger::End;

	if (m_state == NZBFile::MyDecoder::Canceled) return;

	auto size = (std::size_t)(beginInfo.size);
	m_cached_segment = m_file->m_segment_cache.Create(m_file->m_fileID, m_idx, size);
}

void NZBFile::MyDecoder::OnData(const unsigned char* buf, const std::size_t size)
{
	if (m_cached_segment) 
	{
		auto newsize = size;
		if (m_offset + size > m_cached_segment->Size)
		{
			newsize = std::min(size, m_cached_segment->Size > m_offset ? m_cached_segment->Size-m_offset : (std::size_t)0);
			m_file->m_log<<Logger::Error<<"Extra data found when decoding: \"" << m_file->m_beginInfo->name << "\"" << 
			" ignoring " << size - newsize << " bytes" << Logger::End;
		}
		
		if (newsize>0)
			m_cached_segment->Write(buf,m_offset,newsize);
		
		m_offset += newsize;
	}
}

void NZBFile::MyDecoder::OnEndSegment(const yEndInfo& endInfo)
{
	m_state=NZBFile::MyDecoder::End;
	m_file->m_log<<Logger::Debug<<"EndSegment: "<<m_idx<<" "<<m_file->m_beginInfo->name<<Logger::End;
	m_cached_segment.reset();
	m_file->SetSegmentFinished(m_idx);
}

void NZBFile::MyDecoder::OnError(const std::string& msg, const Status& status)
{
	m_state=NZBFile::MyDecoder::Error;
	std::ostringstream errmsg;
	errmsg<<"ErrorInSegment: "<<m_idx+1<<" : "<<msg;
	m_file->m_log<<Logger::Error<<errmsg.str()<<Logger::End;
	m_cached_segment.reset();
	m_file->SetDownloadFailed(m_idx, FileErrorFlag::ErrorInSegment);
}

void NZBFile::MyDecoder::OnWarning(const std::string& msg, const Status& status)
{
	m_file->m_log<<Logger::Warning<<"WarningInSegment: "<<m_idx<<" : "<<msg<<Logger::End;
}

NZBFile::MyArticleDecoder::MyArticleDecoder(std::shared_ptr<NZBFile> file, const std::size_t idx, IFile::CancelSignal* cancel) :
	m_file(file),m_decoder(file,idx)
{
	if (cancel != 0) m_cancel = cancel->connect([this](){ m_decoder.m_state = NZBFile::MyDecoder::Canceled; });
	++m_file->m_decoders;
}

NZBFile::MyArticleDecoder::~MyArticleDecoder()
{
	--m_file->m_decoders;

	if (m_decoder.m_state==NZBFile::MyDecoder::Init || m_decoder.m_state==NZBFile::MyDecoder::Begin)
	{
		std::ostringstream errmsg;
		errmsg<<"NoDataInSegment: "<<m_decoder.m_idx;
		m_file->m_log<<Logger::Error<<errmsg.str()<<Logger::End;
		m_file->SetDownloadFailed(m_decoder.m_idx, FileErrorFlag::EmptySegment);
	}
}

void NZBFile::MyArticleDecoder::OnErrorRetry()
{
	m_file->m_log<<Logger::Warning<<"OnErrorRetry: segment "<<m_decoder.m_idx<<Logger::End;
	m_decoder.Reset();
}
void NZBFile::MyArticleDecoder::OnCancel()
{
	m_file->m_log<<Logger::Info<<"OnCancel: segment "<<m_decoder.m_idx<<Logger::End;
	m_file->SetDownloadFailed(m_decoder.m_idx, FileErrorFlag::OK);
}
void NZBFile::MyArticleDecoder::OnErrorCancel()
{
	m_file->m_log<<Logger::Error<<"OnErrorCancel: segment "<<m_decoder.m_idx<<Logger::End;
	m_file->SetDownloadFailed(m_decoder.m_idx, FileErrorFlag::DownloadCanceled);
}
void NZBFile::MyArticleDecoder::OnData(const char* msg, const std::size_t len)
{
	if (m_decoder.m_state == NZBFile::MyDecoder::Canceled) return;

	try
	{
		m_decoder.Decode(msg, len);
	}
	catch(const std::exception& e)
	{
		m_file->m_log<<Logger::Error<<e.what()<<Logger::End;
		m_file->SetDownloadFailed(m_decoder.m_idx, FileErrorFlag::DecodingFailed);
	}
}
		
void NZBFile::MyArticleDecoder::OnEnd()
{
//	m_file.m_log<<"OnEnd"<<std::endl;

	// OnEnd code is now handled in the destructor...
	
}

void NZBFile::OnStatResponse(const unsigned long idx, const bool have)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	m_checked_segments++;
	if (!have)
	{
		std::ostringstream errmsg;
		errmsg<<"CouldNoFindSegment: "<<idx;
		m_log<<Logger::Error<<errmsg.str()<<Logger::End;

		m_missing_segments++;
		m_segments[idx].status = SegmentState::DownloadFailed;
	}
}

bool NZBFile::Preload(const bool pre_check_segments, CancelSignal* cancel)
{
	for(unsigned long idx=0;idx<m_segments.size();++idx)
	{
		NZBFile::SegmentInfo& info(m_segments[idx]);
		
		if (!info.msgIDs.empty() && !ReadSegment(idx,cancel,true)) break;
		
		if (idx==m_segments.size()-1)
		{
			m_log<<Logger::Error<<"No valid segments in file, preload failed"<<Logger::End;
			return true;
		}
	}	
	
	for(unsigned long idx=0;idx<m_segments.size();++idx)
	{
		NZBFile::SegmentInfo& info(m_segments[idx]);

		if (info.msgIDs.empty())
		{
			std::ostringstream errmsg;
			errmsg<<"Broken NZB-file: Missing segment number "<<idx<<" in NZB-file";
			m_log<<Logger::Error<<errmsg.str()<<Logger::End;

			m_segments[idx].status = SegmentState::DownloadFailed;
			
			return true;
		}
		
		if (pre_check_segments)
		{
// TODO: FIX StatArticle
			if (cancel)
				m_clients.StatArticle(m_groups.back(),info.msgIDs.back(), 
					[this,idx](const bool found){OnStatResponse(idx,found);}, *cancel);
			else
				m_clients.StatArticle(m_groups.back(),info.msgIDs.back(), 
					[this,idx](const bool found){OnStatResponse(idx,found);});
		}
		
		
	}

	return false;
}

bool NZBFile::ReadSegment(const std::size_t idx, CancelSignal* cancel, const bool priority)
{
	assert(idx<m_segments.size());

	m_requested_segments++;

	NZBFile::SegmentInfo& info(m_segments[idx]);
	
	if (info.msgIDs.empty())
	{
		m_log<<Logger::Error<<"No message-ID's in Segment "<<idx<<", skipping"<<Logger::End;
		return true;
	}
	
	SendEvent(m_fileSegmentStateChangedFunction, m_fileID, (int32_t)idx, SegmentState::Loading);

	info.status = SegmentState::Loading;

	auto decoder = std::make_shared<MyArticleDecoder>(shared_from_this(), idx, cancel);
	if (cancel)
		m_clients.GetArticleStream(m_groups, info.msgIDs, decoder, *cancel, priority);
	else
		m_clients.GetArticleStream(m_groups, info.msgIDs, decoder, priority);

	return false;
}

std::filesystem::path NZBFile::GetFileName()
{
	return m_beginInfo ? m_beginInfo->name : "ERROR";
}

unsigned long long NZBFile::GetFileSize()
{
	return m_beginInfo ? m_beginInfo->size : 0;
}


void NZBFile::UpdateVBegin()
{
	static const double f=0.97;
	unsigned long long begin=0;
	
	for(auto& info : m_segments)
	{
		if (info.ypart)
		{
			info.end_boundary=info.ypart->end;
			begin=info.ypart->end;
		}
		else
		{
			begin+=(long)(f*info.msgBytes);
			info.end_boundary=begin;
		}
	}
	
}

std::tuple<std::size_t, std::size_t> NZBFile::FindSegmentIdxRange(const unsigned long long begin, const unsigned long long end)
{
	std::size_t lb, ub, m;
	
	lb=0;
	ub=m_segments.size()-1;

	while (ub-lb>1)
	{
		m=(lb+ub)/2;
		if (begin<m_segments[m].end_boundary) ub=m; else lb=m;
	}

	ub=m_segments.size()-1;

	while (ub-lb>1)
	{
		m=(lb+ub)/2;
		if (end<=m_segments[m].end_boundary) ub=m; else lb=m;
	}

	return {lb, ub};
}

std::tuple<bool, std::size_t> NZBFile::TryGetData(std::unordered_set<std::size_t>& done, char* buf, 
	const unsigned long long offset, const std::size_t size, bool& err_occurred, CancelSignal* cancel, const bool priority)
{
	unsigned long long begin=offset;
	unsigned long long end=offset+size;

	const auto [lb, ub] = FindSegmentIdxRange(begin, end);
	
	bool ready = true;
	std::size_t readsize = 0;
	
	
	for (std::size_t idx = lb; idx <= ub; ++idx)
	{
		const NZBFile::SegmentInfo& info(m_segments[idx]);

		switch (info.status)
		{
		case SegmentState::None:
			err_occurred |= ReadSegment(idx,cancel,priority);
			ready = false;
			continue;
			
		case SegmentState::Loading:
			ready = false;
			continue;
			
		case SegmentState::HasData:
		{
			auto seg_begin = info.ypart->begin-1;
			auto seg_end = info.ypart->end;
			
			if (offset>=seg_end || offset+size<seg_begin) continue;
			
			auto offset1 = seg_begin > offset ? seg_begin - offset : 0;
			auto offset2 = offset > seg_begin ? offset - seg_begin : 0;
			
			auto buf2 = buf + offset1;
			
			auto size2 = std::min((unsigned long long)size - offset1, seg_end - std::max(seg_begin, offset));

			if (done.find(idx)!=done.end())
			{
				readsize+=size2;
				continue;
			}

			auto cached_segment = m_segment_cache.TryGet(m_fileID, idx);
			
//			m_log << Logger::Info << "SegmentCache.TryGet("<<m_fileID<<", "<<idx<<"): "<<(cached_segment?"found":"not found")<<Logger::End;

			if (!cached_segment)
			{
				err_occurred |= ReadSegment(idx,cancel,priority);
				ready = false;
				continue;
			}
			
			readsize += cached_segment->Read(buf2, offset2, size2);
						
			done.insert(idx);

			break;
		}
		default: // Error occurred:
			{
				std::ostringstream errmsg;
				errmsg<<"Error: Missing segment "<<idx+1<<" when reading from file "<<m_beginInfo->name
					<<" ("<<SegmentInfo::Name(info.status)<<").";
				SetErrorFlag(FileErrorFlag::OK); // Error reported elsewhere
				m_log<<Logger::Error<<errmsg.str()<<Logger::End;
				err_occurred = true;
			}
			break;
		}
	}
	
//	m_log<<Logger::Info<<"TryGetData: \""<< m_beginInfo->name <<
//		"\", segments [" << lb << ", " << ub << "], out of [0, "<<m_segments.size()<<"], found "<<
//		done.size()<<" segments; ready="<<(ready?"yes":"no")<<" readsize="<<readsize<<Logger::End;

	return std::make_tuple(ready, readsize);
}

bool NZBFile::GetFileData(char* buf, const unsigned long long offset, const std::size_t size, std::size_t& readsize)
{
	readsize = 0;
	
	if (size==0) return false;
	
	std::unique_lock<std::mutex> lock(m_mutex);

	if (offset>=m_beginInfo->size)
	{
		m_log<<Logger::Debug<<"Failed Reading: "<<m_beginInfo->name<<", "<<offset<<"-"<<offset+size
			<<" (past end)"<<Logger::End;
		return false;
	}
	
	m_log<<Logger::Debug<<"Start Reading: "<<m_beginInfo->name<<", "<<offset<<"-"<<offset+size<<Logger::End;

	bool err=false;
	
	std::unordered_set<std::size_t> done;
	
	std::tuple<bool, std::size_t> trygetres;
	
	while (!std::get<0>(trygetres=TryGetData(done,buf,offset,size,err)) && !err) 
	{
		m_cond.wait(lock);
	}
	
	readsize=std::get<1>(trygetres);
	
	m_log<<Logger::Debug<<"Stop Reading: "<<m_beginInfo->name<<", "<<offset<<"-"<<offset+size
		<<(err?" (failed)": "(succeeded)")<<Logger::End;

	return err;
}

void NZBFile::AsyncGetFileData(OnDataFunction function, char* buf, const unsigned long long offset, const std::size_t size,
	CancelSignal* cancel, const bool priority)
{
	bool ready=false;
	bool err=false;
	std::size_t readsize;
	
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		
		std::unordered_set<std::size_t> done;
		
		std::tuple<bool, std::size_t> trygetres;
		
		if (size==0) ready=true;
		else if (m_beginInfo && offset>=m_beginInfo->size) ready=true;
		else if (std::get<0>(trygetres = TryGetData(done, buf, offset, size, err, cancel, priority)) || err)
		{
			readsize = std::get<1>(trygetres);
			ready = true;
		}
		
		if (!ready) 
		{
			uint_fast64_t id=++m_id_counter;
			boost::signals2::connection conn;
			if (cancel!=0) conn=cancel->connect(std::bind(&NZBFile::CancelAsyncGetFileData,shared_from_this(),id));
			m_data_requests.emplace_back( id, function, buf, offset, size, conn, priority, done );
		}
	}

	if (ready) m_io_service.post( std::bind(function,readsize) );
}

void NZBFile::CancelAsyncGetFileData(const uint_fast64_t& id)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	for(auto i=m_data_requests.begin();i!=m_data_requests.end();++i)
	{
		if (i->id==id)
		{
			m_data_requests.erase(i);
			return;
		}
	}
	m_log<<Logger::Error<<"Failed canceling AsyncGetFileData: "<<id<<Logger::End;
	SetErrorFlag(FileErrorFlag::OK); // ???
}

void NZBFile::ProcessAsyncDataRequests()
{
	std::list< std::tuple< OnDataFunction, std::size_t> > ready_list;
	
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		
		for(auto i=m_data_requests.begin();i!=m_data_requests.end();/*nothing*/)
		{
			AsyncDataRequest& req=*i;

			bool err=false;

			auto trygetres = TryGetData(req.done, req.buf, req.offset, req.size, err, nullptr, req.priority);
			
			if (std::get<0>(trygetres) || err)
			{
				const auto readsize = std::get<1>(trygetres);
				ready_list.emplace_back( std::make_tuple(req.func, readsize) );
				i=m_data_requests.erase(i);
			}
			else
			{
				++i;
			}
		}
	}
	
	for(const auto [func,size] : ready_list)
	{
		func(size);
	}
}

void NZBFile::AsyncGetFilename(OnFilenameFunction func, CancelSignal* cancel)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	
	if (m_beginInfo) func(m_beginInfo->name);
	else
	{
		uint_fast64_t id=++m_id_counter;
		boost::signals2::connection conn;
		if (cancel!=0) conn=cancel->connect(std::bind(&NZBFile::CancelAsyncGetFilename,shared_from_this(),id));
		m_filename_requests.emplace_back( id,func,conn );

		Preload(/*TODO: Pre-check segments*/false,cancel);
	}
}

void NZBFile::CancelAsyncGetFilename(const uint_fast64_t& id)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	for(auto i=m_filename_requests.begin();i!=m_filename_requests.end();++i)
	{
		if (i->id==id)
		{
			m_filename_requests.erase(i);
			return;
		}
	}	
	m_log<<Logger::Error<<"Failed canceling AsyncGetFilename: "<<id<<Logger::End;
	SetErrorFlag(FileErrorFlag::OK); // ???
}

unsigned long long NZBFile::CountMissingBytesInRange(const unsigned long long begin, const unsigned long long end)
{
	const auto [lb, ub] = FindSegmentIdxRange(begin, end);

	unsigned long long count = 0;

	for (std::size_t idx = lb; idx <= ub; ++idx)
	{
		SegmentInfo& segment(m_segments[idx]);
		if (segment.status != SegmentState::HasData) count += segment.msgBytes;
	}

	return count;
}

bool NZBFile::BufferNextSegmentInRange(const unsigned long long begin, const unsigned long long end)
{
	const auto [lb, ub] = FindSegmentIdxRange(begin, end);
	
	for (std::size_t idx = lb; idx <= ub; ++idx)
	{
		if (m_segments[idx].status == SegmentState::None)
		{
			ReadSegment(idx,0);
			return true;
		}
	}
	
	return false;
}


}


