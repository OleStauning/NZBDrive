/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef NZBDRIVE_H
#define NZBDRIVE_H
#include "INZBDrive.hpp"
#include "SegmentState.hpp"
#include "ContentType.hpp"
#include "ConnectionState.hpp"
#include "nzb.hpp"
#include <stdint.h>
#include <memory>
#include <string>
#include <functional>
#include <filesystem>
#include "make_copyable.hpp"

namespace ByteFountain
{
	class Logger;

	class NZBDrive : public INZBDrive
	{
	public:
		
		NZBDrive();
		NZBDrive(Logger& log);
		virtual ~NZBDrive();

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

		void SetNetworkThrottling(const NetworkThrottling& shape);

		int32_t AddServer(const UsenetServer& server);
		void RemoveServer(const int32_t id);
		
		bool Start();
		void Stop();


		template <class FuncT>
		int32_t Mount(const std::filesystem::path& mountdir, const std::string& nzbfile, FuncT&& func, const MountOptions mountOptions = MountOptions::Default)
		{
			if constexpr (std::is_copy_constructible<FuncT>::value) 
				return _Mount(mountdir, nzbfile, std::move(func), mountOptions);
			else 
				return _Mount(mountdir, nzbfile, make_copyable(std::move(func)), mountOptions);
		}

		template <class FuncT>
		int32_t Mount(const std::filesystem::path& mountdir, const nzb& nzb, FuncT&& func, const MountOptions mountOptions = MountOptions::Default)
		{
			if constexpr (std::is_copy_constructible<FuncT>::value) 
				return _Mount(mountdir, nzb, std::move(func), mountOptions);
			else 
				return _Mount(mountdir, nzb, make_copyable(std::move(func)), mountOptions);
		}

		int32_t Unmount(const std::filesystem::path& mountdir);

		std::shared_ptr<IDirectory> GetRootDir();
		std::shared_ptr<IDirectory> GetDirectory(const std::filesystem::path& p);
		std::shared_ptr<IFile> GetFile(const std::filesystem::path& p);
		std::vector< std::pair<std::filesystem::path, ContentType> > GetContent();
		void EnumFiles(std::function<void (const std::filesystem::path& path, std::shared_ptr<IFile> file)> callback);

		uint_fast64_t RXBytes();
		uint_fast64_t GetTotalNumberOfBytes() const;

	private:
		
		int32_t _Mount(const std::filesystem::path& mountdir, const std::string& nzbfile, 
			const MountStatusFunction& mountStatusFunction, const MountOptions mountOptions = MountOptions::Default);
		int32_t _Mount(const std::filesystem::path& mountdir, const nzb& nzb, 
			const MountStatusFunction& mountStatusFunction, const MountOptions mountOptions = MountOptions::Default);
		
		
		NZBDrive& operator =(const NZBDrive&) = delete;
		NZBDrive(const NZBDrive&) = delete;
		std::unique_ptr<class NZBDriveIMPL> m_pImpl;
	};
	
}
#endif
