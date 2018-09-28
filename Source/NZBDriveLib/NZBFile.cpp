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
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.


namespace ByteFountain
{

template <typename T, typename... TArgs>
static void SendEvent(T& handler, TArgs&&... args)
{
	auto h = handler;
	if (h) h(std::forward<TArgs>(args)...);
}


NZBFile::SegmentInfo::SegmentInfo():
msgIDs(), has_ypart(false), ypart(), status(SegmentState::None)
{}
	
NZBFile::NZBFile(boost::asio::io_service& io_service, const int32_t nzbID, const int32_t fileID, 
	const nzb::file& nzbfile, NewsClientCache& clients, const boost::filesystem::path& cache_path, Logger& log,
	NZBFile::FileAddedFunction fileAddedFunction,
	NZBFile::FileInfoFunction fileInfoFunction,
	NZBFile::FileSegmentStateChangedFunction fileSegmentStateChangedFunction,
	NZBFile::FileRemovedFunction fileRemovedFunction
	) :
	m_io_service(io_service),
	m_nzbID(nzbID),
	m_fileID(fileID),
	m_clients(clients),
	m_cache_path(cache_path),
	m_cache_diskfile(m_cache_path/boost::lexical_cast<std::string>(boost::uuids::random_generator()())),
	m_cache_file(),
	m_groups(nzbfile.groups.begin(),nzbfile.groups.end()),
	m_checked_segments(0),
	m_missing_segments(0),
	m_requested_segments(0),
	m_hasBeginInfo(false),
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
			continue;
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
		m_segments[s.number-1].bytes=s.bytes; // TODO: Maybe redundant segments have different sizes??
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

		bytes += s.bytes;
	}

	SendEvent(m_fileAddedFunction, m_nzbID, m_fileID, (int32_t)m_segments.size(), bytes);

	UpdateVBegin();
}

NZBFile::~NZBFile()
{
	SendEvent(m_fileRemovedFunction, m_fileID);
	
//	m_log<<"~NZBFile"<<std::endl;

//	std::cout<<"~NZBFile "<<m_decoders<<std::endl;

	assert(m_decoders>=0);
	assert(m_decoders==0);

//	std::cout<<"REMOVING CACHE-FILE: "<<m_cache_diskfile<<std::endl;
	/*
	boost::filesystem::remove(m_cache_diskfile);

	if (boost::filesystem::exists(m_cache_diskfile))
	{
		m_log<<Logger::Error<<"Could not remove cache file "<<m_cache_diskfile<<", file is locked??"<<Logger::End;
	}
	*/
}


bool NZBFile::SetBeginInfo(const yBeginInfo& beginInfo)	
{
	if (!m_hasBeginInfo)
	{
		m_hasBeginInfo=true;
		m_beginInfo=beginInfo;
//		m_beginInfo.name = text_tool::cp_to_utf8(text_tool::cp1252, m_beginInfo.name);
//		boost::trim(m_beginInfo.name);
		// Generate empty file:
//		m_log<<Logger::Info<<"CREATING CACHE-FILE: "<<m_cache_diskfile<<Logger::End;

		SendEvent(m_fileInfoFunction, m_fileID, m_beginInfo.name, m_beginInfo.size);

		try
		{
			m_cache_file.AllocateCacheFile(m_cache_diskfile.string(),m_beginInfo.size);
		}
		catch(const std::exception& e)
		{
			// TODO: IN CASE WE CANNOT ALLOCATE THE FILE WE HAVE TO SKIP IT IN THE MOUNTING PROCESS.
			m_log<<Logger::Error<<e.what()<<Logger::End;
			SetErrorFlag(FileErrorFlag::CacheFileError);
		}

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
		m_segments[idx].has_ypart = true;

		m_cond.notify_all();
	}
	if (newfile)
	{
		for(auto req : m_filename_requests) m_io_service.post( std::bind(req->func,m_beginInfo.name) );
		m_filename_requests.clear();
	}
}

void NZBFile::SetFileInfo(const yBeginInfo& beginInfo)
{
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		SetBeginInfo(beginInfo);

		m_cond.notify_all();
	}
	for(auto req : m_filename_requests) m_io_service.post( std::bind(req->func,m_beginInfo.name) );
	m_filename_requests.clear();
}

void NZBFile::SetDownloadFinished(const std::size_t idx)
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

	if (!m_hasBeginInfo)
	{
		for(auto req : m_filename_requests) m_io_service.post( std::bind(req->func,"") );
		m_filename_requests.clear();
	}
}

NZBFile::MyDecoder::MyDecoder(std::shared_ptr<NZBFile> file, const std::size_t idx) :
	m_state(NZBFile::MyDecoder::Init),
	m_file(file),
	m_decoder(*this),
	m_idx(idx),
	m_diskfile(0)
{
}
NZBFile::MyDecoder::~MyDecoder()
{
	m_file->m_cache_file.CloseCacheFile(m_diskfile);
	assert(m_diskfile==0);
}

void NZBFile::MyDecoder::Decode(const char* yenc_data, const std::size_t len) { m_decoder.ReadLine(yenc_data, len); }
void NZBFile::MyDecoder::Reset() { m_decoder.Reset(); }

yCacheFileHandle NZBFile::MyDecoder::OnBeginSegment(const yBeginInfo& beginInfo, const yPartInfo& partInfo)
{
	m_state=NZBFile::MyDecoder::Begin;
	m_file->SetFileInfo(m_idx,beginInfo,partInfo);

	if (m_file->m_beginInfo.size != beginInfo.size)
	{
		m_file->m_log<<Logger::Error<<"=ybegin size parameter mismatch when reading the parts of: \"" << m_file->m_beginInfo.name << "\"" << Logger::End;
	}
	if (m_file->m_beginInfo.size < partInfo.end)
	{
		m_file->m_log<<Logger::Error<<"=ybegin size parameter less than =ypart end parameter: \"" << m_file->m_beginInfo.name << Logger::End;
		

		m_file->m_cache_file.CloseCacheFile(m_diskfile);
	}
	else
	{
		m_file->m_log<<Logger::Info<<"BeginSegment: "<<m_idx<<" "<<partInfo.begin-1<<"-"<<partInfo.end-1<<" "<<m_file->m_beginInfo.name<<Logger::End;

		if (m_state == NZBFile::MyDecoder::Canceled) return nullptr;

		m_file->m_cache_file.OpenCacheFileForWrite(partInfo.begin - 1, (std::size_t)(partInfo.end - partInfo.begin + 1), m_diskfile);
	}

	return m_diskfile;
}
yCacheFileHandle NZBFile::MyDecoder::OnBeginSegment(const yBeginInfo& beginInfo)
{
	m_state=NZBFile::MyDecoder::Begin;
	m_file->SetFileInfo(beginInfo);

	if (m_file->m_beginInfo.size != beginInfo.size)
	{
		m_file->m_log<<Logger::Error<<"=ybegin size parameter mismatch when reading the parts of: \"" << m_file->m_beginInfo.name << "\"" << Logger::End;
	}
		
	m_file->m_log<<Logger::Info<<"BeginSegment: "<<m_idx<<" 0-"<<beginInfo.size-1<<" "<<m_file->m_beginInfo.name<<Logger::End;

	if (m_state == NZBFile::MyDecoder::Canceled) return nullptr;

	m_file->m_cache_file.OpenCacheFileForWrite(0, (std::size_t)(beginInfo.size), m_diskfile);

	return m_diskfile;
}

void NZBFile::MyDecoder::OnEndSegment(const yEndInfo& endInfo)
{
	m_state=NZBFile::MyDecoder::End;
	m_file->m_log<<Logger::Info<<"EndSegment: "<<m_idx<<" "<<m_file->m_beginInfo.name<<Logger::End;
	m_file->m_cache_file.CloseCacheFile(m_diskfile);
	m_file->SetDownloadFinished(m_idx);
}

void NZBFile::MyDecoder::OnError(const std::string& msg, const Status& status)
{
	m_state=NZBFile::MyDecoder::Error;
	std::ostringstream errmsg;
	errmsg<<"ErrorInSegment: "<<m_idx<<" : "<<msg;
	m_file->m_log<<Logger::Error<<errmsg.str()<<Logger::End;
	m_file->m_cache_file.CloseCacheFile(m_diskfile);
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
		errmsg<<"CouldNoFindSegment: "<<idx+1;
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

		if (info.msgIDs.empty())
		{
			std::ostringstream errmsg;
			errmsg<<"Broken NZB-file: Missing segment number "<<idx+1<<" in NZB-file";
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
		
		if (idx==0 && ReadSegment(idx,cancel,true))
		{
			SetDownloadFailed(idx, FileErrorFlag::OK); // Error would be set elsewhere.
			return true;
		}
		
	}

	return false;
}

bool NZBFile::ReadSegment(const std::size_t idx, CancelSignal* cancel, const bool priority)
{
	assert(idx<m_segments.size());

	m_requested_segments++;

	NZBFile::SegmentInfo& info(m_segments[idx]);
	
	if (info.status == SegmentState::None)
	{
		SendEvent(m_fileSegmentStateChangedFunction, m_fileID, (int32_t)idx, SegmentState::Loading);

		info.status = SegmentState::Loading;
		// If shared_from_this fails in the following line, it is propably because this object
		// needs to have a shared_ptr pointing to it...
		std::shared_ptr<MyArticleDecoder> decoder(new MyArticleDecoder(shared_from_this(), idx, cancel));
		if (cancel)
			m_clients.GetArticleStream(m_groups, info.msgIDs, decoder, *cancel, priority);
		else
			m_clients.GetArticleStream(m_groups, info.msgIDs, decoder, priority);

		return false;
	}

	return info.status != SegmentState::HasData; // returns true on errors
}

/*
bool NZBFile::AssertBeginInfo()
{
	std::unique_lock<std::mutex> lock(m_mutex);

	while(!m_hasBeginInfo && !m_segments[0].has_error)
	{
		if (ReadSegment(0)) return true;
		m_cond.wait(lock);
	}
	return m_segments[0].has_error;
}
*/
boost::filesystem::path NZBFile::GetFileName()
{
//	if (AssertBeginInfo()) return "ERROR";
	return m_beginInfo.name;	
}

unsigned long long NZBFile::GetFileSize()
{
//	if (AssertBeginInfo()) return 0;
	return m_beginInfo.size;
}


void NZBFile::UpdateVBegin()
{
	static const double f=0.97;
	unsigned long long begin=0;
	
	for(unsigned long idx=0;idx<m_segments.size();++idx)
	{
		NZBFile::SegmentInfo& info(m_segments[idx]);

		if (info.has_ypart)
		{
			info.end_boundary=info.ypart.end;
//			m_seg_boundaries[idx]=info.ypart.begin-1;
			begin=info.ypart.end;
		}
		else
		{
			begin+=(long)(f*info.bytes);
			info.end_boundary=begin;
		}
	}
	
}

std::pair<std::size_t, std::size_t> NZBFile::FindSegmentIdxRange(const unsigned long long begin, const unsigned long long end)
{
	std::pair<std::size_t, std::size_t> res;

	std::size_t lb, ub, m;
	
	lb=0;
	ub=m_segments.size()-1;

	while (ub-lb>1)
	{
		m=(lb+ub)/2;
		if (begin<m_segments[m].end_boundary) ub=m; else lb=m;
	}

	res.first = lb;

	ub=m_segments.size()-1;

	while (ub-lb>1)
	{
		m=(lb+ub)/2;
		if (end<=m_segments[m].end_boundary) ub=m; else lb=m;
	}

	res.second = ub;

	return res;
}

std::size_t NZBFile::TryGetData(char* buf, const unsigned long long offset, const std::size_t size,
	bool& err_occurred, CancelSignal* cancel, const bool priority)
{
	unsigned long long begin=offset;
	unsigned long long end=offset+size;

	std::pair<std::size_t, std::size_t> range = FindSegmentIdxRange(begin, end);
	
	bool ready=true;

	for (std::size_t idx = range.first; idx <= range.second; ++idx)
	{
		const NZBFile::SegmentInfo& info(m_segments[idx]);

		switch (info.status)
		{
		case SegmentState::None:
			err_occurred |= ReadSegment(idx,cancel,priority);
			// (Intentionally fall through)
		case SegmentState::Loading:
			ready = false;
			// (Intentionally fall through)
		case SegmentState::HasData:
			break;
		default: // Error occurred:
			{
				std::ostringstream errmsg;
				errmsg<<"Error: Missing segment "<<idx<<" when reading from file "<<m_beginInfo.name
					<<" ("<<SegmentInfo::Name(info.status)<<").";
				SetErrorFlag(FileErrorFlag::OK); // Error reported elsewhere
				m_log<<Logger::Error<<errmsg.str()<<Logger::End;
				err_occurred = true;
			}
			break;
		}
	}
	
	if (!ready) return 0;
	
	std::size_t readsize = size;
	if (readsize>m_beginInfo.size - offset) readsize = (std::size_t)(m_beginInfo.size - offset);
	
	if (buf) readsize = m_cache_file.ReadFromCacheFile(buf, offset, readsize);

	return readsize;
}

bool NZBFile::GetFileData(char* buf, const unsigned long long offset, const std::size_t size, std::size_t& readsize)
{
	std::unique_lock<std::mutex> lock(m_mutex);

	if (size==0 || offset>=m_beginInfo.size)
	{
		m_log<<Logger::Debug<<"Failed Reading: "<<m_beginInfo.name<<", "<<offset<<"-"<<offset+size
			<<" (past end)"<<Logger::End;
		return false;
	}
	
	m_log<<Logger::Debug<<"Start Reading: "<<m_beginInfo.name<<", "<<offset<<"-"<<offset+size<<Logger::End;

	bool err=false;
	
	while (0==(readsize=TryGetData(buf,offset,size,err)) && !err) 
	{
		m_cond.wait(lock);
	}
		
	m_log<<Logger::Debug<<"Stop Reading: "<<m_beginInfo.name<<", "<<offset<<"-"<<offset+size
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
		
		if (size==0) ready=true;
		else if (m_hasBeginInfo && offset>=m_beginInfo.size) ready=true;
		else if (0 != (readsize = TryGetData(buf, offset, size, err, cancel, priority)) || err) ready = true;
	
		if (!ready) 
		{
			uint_fast64_t id=++m_id_counter;
			boost::signals2::connection conn;
			if (cancel!=0) conn=cancel->connect(std::bind(&NZBFile::CancelAsyncGetFileData,shared_from_this(),id));
			m_data_requests.emplace_back( new AsyncDataRequest( id, function, buf, offset, size, conn, priority ) );
		}
	}

	if (ready) m_io_service.post( std::bind(function,readsize) );
}

void NZBFile::CancelAsyncGetFileData(const uint_fast64_t& id)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	for(auto i=m_data_requests.begin();i!=m_data_requests.end();++i)
	{
		if ((*i)->id==id)
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
	std::list< std::pair< std::shared_ptr<AsyncDataRequest>, std::size_t> > ready_list;
	
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		
		for(auto i=m_data_requests.begin();i!=m_data_requests.end();/*nothing*/)
		{
			const std::shared_ptr<AsyncDataRequest>& req=*i;

			bool err=false;

			std::size_t readsize = TryGetData(req->buf, req->offset, req->size, err, nullptr, req->priority);
			
			if (readsize>0 || err)
			{
				ready_list.emplace_back(std::make_pair(req,readsize));
				i=m_data_requests.erase(i);
			}
			else
			{
				++i;
			}
		}
	}
	
	for(const auto& r : ready_list)
	{
		r.first->func(r.second);
	}
}

void NZBFile::AsyncGetFilename(OnFilenameFunction func, CancelSignal* cancel)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	
	if (m_hasBeginInfo) func(m_beginInfo.name);
	else
	{
		uint_fast64_t id=++m_id_counter;
		boost::signals2::connection conn;
		if (cancel!=0) conn=cancel->connect(std::bind(&NZBFile::CancelAsyncGetFilename,shared_from_this(),id));
		m_filename_requests.emplace_back( new AsyncFilenameRequest( id,func,conn ) );

		Preload(/*TODO: Pre-check segments*/false,cancel);
	}
}

void NZBFile::CancelAsyncGetFilename(const uint_fast64_t& id)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	for(auto i=m_filename_requests.begin();i!=m_filename_requests.end();++i)
	{
		if ((*i)->id==id)
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
	std::pair<std::size_t, std::size_t> range = FindSegmentIdxRange(begin, end);

	unsigned long long count = 0;

	for (std::size_t idx = range.first; idx <= range.second; ++idx)
	{
		SegmentInfo& segment(m_segments[idx]);
		if (segment.status != SegmentState::HasData) count += segment.bytes;
	}

	return count;
}

bool NZBFile::BufferNextSegmentInRange(const unsigned long long begin, const unsigned long long end)
{
	std::pair<std::size_t, std::size_t> range = FindSegmentIdxRange(begin, end);
	
	for (std::size_t idx = range.first; idx <= range.second; ++idx)
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


