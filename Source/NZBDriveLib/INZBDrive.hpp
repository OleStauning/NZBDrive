/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef INZBDRIVE_H
#define INZBDRIVE_H
#include "SegmentState.hpp"
#include "ContentType.hpp"
#include "ConnectionState.hpp"
#include "LogLineLevel.hpp"
#include <stdint.h>
#include <memory>
#include <string>
#include <functional>
#include <boost/filesystem.hpp>

namespace ByteFountain
{
	class IFile;
	class IDirectory;

	struct UsenetServer
	{
		std::string ServerName;
		std::string UserName;
		std::string Password;
		std::string Port;
		bool UseSSL;
		unsigned int Connections;
		unsigned int Priority;
		unsigned int Timeout;
		unsigned int Pipeline;

		UsenetServer(){}

		UsenetServer(
			const std::string serverName,
			const std::string userName,
			const std::string password,
			const std::string port,
			const bool useSSL,
			const unsigned int connections,
			const unsigned int priority,
			const unsigned int timeout,
			const unsigned int pipeline) :
			ServerName(serverName),
			UserName(userName),
			Password(password),
			Port(port),
			UseSSL(useSSL),
			Connections(connections),
			Priority(priority),
			Timeout(timeout),
			Pipeline(pipeline)
		{}
	};

	struct NetworkThrottling
	{
		enum NetworkMode { Constant, Adaptive } Mode;

		std::size_t NetworkLimit;
		std::size_t FastPreCache;
		std::size_t AdaptiveIORatioPCT; // Output to Input ratio in percent to use in Adaptive mode
		std::size_t BackgroundNetworkRate;

		NetworkThrottling() :
			Mode(Constant),
			NetworkLimit(0),
			FastPreCache(10 * 1024 * 1024),
			AdaptiveIORatioPCT(110/* percent */),
			BackgroundNetworkRate(1000)
		{}
	};

	enum MountErrorCode
	{
		LoadNZBFileError = -2,
		InvalidNZBFileError = -3,
		CacheDirFullError = -4,
		InvalidCacheDir = -5,
		MountingFailed = -6,
		UnmountingFailed = -7,
		MountErrorCodeEND = -8 //<-- MUST BE THE LAST ENTRY
	};

	enum class MountOptions : int
	{
		Default = 0x000,
		DontExtractArchives = 0x001
	};
	
	inline MountOptions operator|(const MountOptions lhs, const MountOptions rhs) 
	{
		return static_cast<MountOptions>(static_cast<int>(lhs) | static_cast<int>(rhs));
	}
	inline MountOptions operator&(const MountOptions lhs, const MountOptions rhs) 
	{
		return static_cast<MountOptions>(static_cast<int>(lhs) & static_cast<int>(rhs));
	}
	
	typedef std::function< void(const LogLineLevel lvl, const std::string& msg)> LoggingFunction;
	typedef std::function< void(const ConnectionState state, const int32_t server, const int32_t thread) > ConnectionStateChangedFunction;
	typedef std::function< void(const int64_t time, const int32_t server, const uint64_t bytesTX, uint64_t bytesRX,
		uint32_t missingSegmentCount, uint32_t connectionTimeoutCount, uint32_t errorCount)> ConnectionInfoFunction;
	typedef std::function< void(const int32_t nzbID, const int32_t parts, const int32_t total) > MountStatusFunction;
	typedef std::function< void(const int32_t nzbID, const boost::filesystem::path& path) > NZBFileOpenFunction;
	typedef std::function< void(const int32_t nzbID) > NZBFileCloseFunction;
	typedef std::function< void(const int32_t nzbID, const int32_t fileID, const int32_t segments, const uint64_t size) > NZBFileAddedFunction;
	typedef std::function< void(const int32_t fileID, const boost::filesystem::path& name, const uint64_t size) > NZBFileInfoFunction;
	typedef std::function< void(const int32_t fileID, const int32_t segment, const SegmentState state) > NZBFileSegmentStateChangedFunction;
	typedef std::function< void(const int32_t fileID) > NZBFileRemovedFunction;

	class INZBDrive
	{
	public:
		virtual void ResetHandlers() = 0;
		virtual void SetLoggingHandler(LoggingFunction handler) = 0;
		virtual void SetConnectionStateChangedHandler(ConnectionStateChangedFunction handler) = 0;
		virtual void SetConnectionInfoHandler(ConnectionInfoFunction handler) = 0;
		virtual void SetNZBFileOpenHandler(NZBFileOpenFunction nzbFileOpenFunction) = 0;
		virtual void SetNZBFileCloseHandler(NZBFileCloseFunction nzbFileCloseFunction) = 0;
		virtual void SetFileAddedHandler(NZBFileAddedFunction fileAddedFunction) = 0;
		virtual void SetFileInfoHandler(NZBFileInfoFunction fileInfoFunction) = 0;
		virtual void SetFileSegmentStateChangedHandler(NZBFileSegmentStateChangedFunction fileSegmentStateChangedFunction) = 0;
		virtual void SetFileRemovedHandler(NZBFileRemovedFunction fileRemovedFunction) = 0;

		virtual void SetLoggingLevel(const LogLineLevel level) = 0;

		virtual void SetNetworkThrottling(const NetworkThrottling& shape) = 0;

		virtual int32_t AddServer(const UsenetServer& server) = 0;
		virtual void RemoveServer(const int32_t id) = 0;

		virtual bool Start() = 0;
		virtual void Stop() = 0;

		virtual int32_t Mount(const boost::filesystem::path& mountdir, const std::string& nzbfile, MountStatusFunction mountStatusFunction, const MountOptions mountOptions = MountOptions::Default) = 0;
		virtual int32_t Unmount(const boost::filesystem::path& mountdir) = 0;

		virtual std::shared_ptr<IDirectory> GetRootDir() = 0;
		virtual std::shared_ptr<IDirectory> GetDirectory(const boost::filesystem::path& p) = 0;
		virtual std::shared_ptr<IFile> GetFile(const boost::filesystem::path& p) = 0;
		virtual std::vector< std::pair<boost::filesystem::path, ContentType> > GetContent() = 0;
		virtual void EnumFiles(std::function<void(const boost::filesystem::path& path, std::shared_ptr<IFile> file)> callback) = 0;

		virtual uint_fast64_t RXBytes() = 0;
		virtual uint_fast64_t GetTotalNumberOfBytes() const = 0;

	};

}
#endif
