/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef NZBDRIVEIMPL_HPP
#define NZBDRIVEIMPL_HPP
#define _CRT_SECURE_NO_WARNINGS

#include "NZBDrive.hpp"
#include "NZBFile.hpp"
#include "NZBDirectory.hpp"
#include "Logger.hpp"
#include "NZBDriveMounter.hpp"
#include "nzb_fileloader.hpp"
#include "Logger.hpp"
#include "ReadAheadFile.hpp"

#include <thread>

namespace ByteFountain
{

	struct DriveStats
	{
		std::atomic<int> BlockedReads;
		std::atomic<uint_fast64_t> RXBytes;
		std::atomic<uint_fast64_t> FastReadUntil;
		DriveStats() :BlockedReads(0), RXBytes(0), FastReadUntil(0){}
	};


	class NZBDriveIMPL
	{
		static const int64_t ms_timeout = 15 * 60 * 1000;
	private:

		enum State { Stopped, Timeout, Started };

		Logger m_defaultLogger;
		Logger& m_logger;
		State m_state;
		std::mutex m_mutex;
		std::chrono::steady_clock::time_point m_startTime;

		typedef boost::signals2::signal<void()> CancelSignal;

		typedef std::map< const boost::filesystem::path, std::shared_ptr<NZBDriveMounter> > MountStates;
		MountStates m_mountStates;

		friend struct NZBDriveMounter;

		bool m_registered;
		boost::asio::io_service m_io_service;
		RateLimiter m_limiter;
		uint_fast64_t m_LastDriveRXBytes;
		NewsClientCache m_clients;
		std::thread m_thread;
		std::unique_ptr<boost::asio::io_service::work> m_work;

		boost::filesystem::path m_cache_path;

		std::shared_ptr<NZBDirectory> m_root_dir;
		std::shared_ptr<NZBDirectory> m_root_rawdir;

		boost::asio::deadline_timer m_timer;
		time_duration m_duration;

		void HeartBeat(const boost::system::error_code& e);
		void MyTrialTimeout();

		Logger& m_log;
		std::atomic<int32_t> m_nzbCount;
		std::atomic<int32_t> m_fileCount;

		NetworkThrottling m_shape;
		NetworkThrottling::NetworkMode m_currentThrottlingMode;
		std::size_t m_readRate;
		std::shared_ptr<ReadAheadFile> m_readAheadFile;

		DriveStats m_driveStats;

		// Event handling:
		boost::asio::io_service m_event_io_service;
		boost::asio::strand m_event_strand;
		std::thread m_event_thread;
		std::unique_ptr<boost::asio::io_service::work> m_event_work;

		NZBFileOpenFunction m_nzbFileOpenFunction;
		NZBFileCloseFunction m_nzbFileCloseFunction;
		NZBFileAddedFunction m_fileAddedFunction;
		NZBFileInfoFunction m_fileInfoFunction;
		NZBFileSegmentStateChangedFunction m_fileSegmentStateChangedFunction;
		NZBFileRemovedFunction m_fileRemovedFunction;
		// Event handling.


		friend class ReadAheadFile;
		void StartRead(const std::size_t size) { m_driveStats.BlockedReads++; m_driveStats.RXBytes += size; }
		void StopRead() { m_driveStats.BlockedReads--; }
		void PrecacheMissingBytes(std::shared_ptr<ReadAheadFile> readAheadFile, const unsigned long long missingBytes)
		{
			m_driveStats.FastReadUntil = m_limiter.Network.RXBytes + missingBytes;

#ifdef _MSC_VER			
			std::atomic_store(&m_readAheadFile, readAheadFile);
#else
			m_readAheadFile = readAheadFile;
#endif
		}

		NZBDriveIMPL(Logger* log);


		RateLimiter::Parameters GetRateLimiterConfiguration();

		int32_t Mount(const int32_t mountID, const boost::filesystem::path& expanded_cache_path, 
			const boost::filesystem::path& mountdir, const nzb& nzb, 
			MountStatusFunction mountStatusFunction, const MountOptions mountOptions);
		NZBDriveIMPL::MountStates::iterator Unmount(const NZBDriveIMPL::MountStates::iterator& it_state);

	protected:

		virtual void _Stop();

	public:

		NZBDriveIMPL& operator =(const NZBDriveIMPL&) = delete;
		NZBDriveIMPL(const NZBDriveIMPL&) = delete;

		NZBDriveIMPL();
		NZBDriveIMPL(Logger& log);
		virtual ~NZBDriveIMPL();

		void ResetHandlers();
		void SetLoggingHandler(LoggingFunction handler);
		void SetConnectionStateChangedHandler(ConnectionStateChangedFunction handler);
		void SetConnectionInfoHandler(ConnectionInfoFunction handler);
		void SetNZBFileOpenHandler(NZBFileOpenFunction nzbFileOpenFunction);
		void SetNZBFileCloseHandler(NZBFileCloseFunction nzbFileCloseFunction);
		void SetFileAddedHandler(NZBFileAddedFunction fileAddedFunction);
		void SetFileInfoHandler(NZBFileInfoFunction fileInfoFunction);
		void SetFileSegmentStateChangedHandler(NZBFileSegmentStateChangedFunction fileSegmentStateChangedFunction);
		void SetFileRemovedHandler(NZBFileRemovedFunction fileRemovedFunction);

		void SetLoggingLevel(const LogLineLevel level);

		void SetCachePath(const boost::filesystem::path& cache_path);
		boost::filesystem::path GetCachePath() const;

		void SetNetworkThrottling(const NetworkThrottling& shape) { m_shape = shape; }

		int32_t AddServer(const UsenetServer& server);
		void RemoveServer(const int32_t id);

		bool Start();
		void Stop();

		int32_t Mount(const boost::filesystem::path& mountdir, const std::string& nzbfile, 
			MountStatusFunction mountStatusFunction, const MountOptions mountOptions);
		int32_t Mount(const boost::filesystem::path& mountdir, const std::string& nzbfile, 
			const nzb& nzb, MountStatusFunction mountStatusFunction, const MountOptions mountOptions);
		int32_t Unmount(const boost::filesystem::path& mountdir);

		std::shared_ptr<IDirectory> GetRootDir();
		std::shared_ptr<IDirectory> GetDirectory(const boost::filesystem::path& p);
		std::shared_ptr<IFile> GetFile(const boost::filesystem::path& p);
		std::vector< std::pair<boost::filesystem::path, ContentType> > GetContent();
		void EnumFiles(std::function<void(const boost::filesystem::path& path, std::shared_ptr<IFile> file)> callback);

		uint_fast64_t RXBytes() const { return m_limiter.Network.RXBytes; }
		uint_fast64_t GetTotalNumberOfBytes() const;

	};


}

#endif
