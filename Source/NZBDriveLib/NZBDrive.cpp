/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#define _CRT_SECURE_NO_WARNINGS

#include "NZBDrive.hpp"
#include "NZBFile.hpp"
#include "NZBDirectory.hpp"
#include "Logger.hpp"
#include "NZBDriveMounter.hpp"
#include "nzb_fileloader.hpp"
#include "Logger.hpp"
#include "ReadAheadFile.hpp"
#include "NZBDriveIMPL.hpp"
#include <thread>

namespace ByteFountain
{

	
	NZBDrive::NZBDrive()
	{
		m_pImpl.reset(new NZBDriveIMPL());
	}
	NZBDrive::NZBDrive(Logger& log)
	{
		m_pImpl.reset(new NZBDriveIMPL(log));
	}
	NZBDrive::~NZBDrive()
	{}

	void NZBDrive::ResetHandlers()
	{
		m_pImpl->ResetHandlers();
	}
	void NZBDrive::SetLoggingHandler(LoggingFunction handler)
	{
		m_pImpl->SetLoggingHandler(handler);
	}
	void NZBDrive::SetConnectionStateChangedHandler(ConnectionStateChangedFunction handler)
	{
		m_pImpl->SetConnectionStateChangedHandler(handler);
	}
	void NZBDrive::SetConnectionInfoHandler(ConnectionInfoFunction handler)
	{
		m_pImpl->SetConnectionInfoHandler(handler);
	}
	void NZBDrive::SetNZBFileOpenHandler(NZBFileOpenFunction nzbFileOpenFunction)
	{
		m_pImpl->SetNZBFileOpenHandler(nzbFileOpenFunction);
	}
	void NZBDrive::SetNZBFileCloseHandler(NZBFileCloseFunction nzbFileCloseFunction)
	{
		m_pImpl->SetNZBFileCloseHandler(nzbFileCloseFunction);
	}
	void NZBDrive::SetFileAddedHandler(NZBFileAddedFunction fileAddedFunction)
	{
		m_pImpl->SetFileAddedHandler(fileAddedFunction);
	}
	void NZBDrive::SetFileInfoHandler(NZBFileInfoFunction fileInfoFunction)
	{
		m_pImpl->SetFileInfoHandler(fileInfoFunction);
	}
	void NZBDrive::SetFileSegmentStateChangedHandler(NZBFileSegmentStateChangedFunction fileSegmentStateChangedFunction)
	{
		m_pImpl->SetFileSegmentStateChangedHandler(fileSegmentStateChangedFunction);
	}
	void NZBDrive::SetFileRemovedHandler(NZBFileRemovedFunction fileRemovedFunction)
	{
		m_pImpl->SetFileRemovedHandler(fileRemovedFunction);
	}
	void NZBDrive::SetLoggingLevel(const LogLineLevel level)
	{
		m_pImpl->SetLoggingLevel(level);
	}
	void NZBDrive::SetNetworkThrottling(const NetworkThrottling& shape)
	{
		m_pImpl->SetNetworkThrottling(shape);
	}
	int32_t NZBDrive::AddServer(const UsenetServer& server)
	{
		return m_pImpl->AddServer(server);
	}
	void NZBDrive::RemoveServer(const int32_t id)
	{
		m_pImpl->RemoveServer(id);
	}
	bool NZBDrive::Start()
	{
		return m_pImpl->Start();
	}
	void NZBDrive::Stop()
	{
		m_pImpl->Stop();
	}
	int32_t NZBDrive::Mount(const std::filesystem::path& mountdir, const std::string& nzbfile, 
		MountStatusFunction mountStatusFunction, const MountOptions mountOptions)
	{
		return m_pImpl->Mount(mountdir, nzbfile, mountStatusFunction, mountOptions);
	}
	int32_t NZBDrive::Mount(const std::filesystem::path& mountdir, const nzb& nzb, 
		MountStatusFunction mountStatusFunction, const MountOptions mountOptions)
	{
		return m_pImpl->Mount(mountdir, "", nzb, mountStatusFunction, mountOptions);
	}
	int32_t NZBDrive::Unmount(const std::filesystem::path& mountdir)
	{
		return m_pImpl->Unmount(mountdir);
	}
	std::shared_ptr<IDirectory> NZBDrive::GetRootDir()
	{
		return m_pImpl->GetRootDir();
	}
	std::shared_ptr<IDirectory> NZBDrive::GetDirectory(const std::filesystem::path& p)
	{
		return m_pImpl->GetDirectory(p);
	}
	std::shared_ptr<IFile> NZBDrive::GetFile(const std::filesystem::path& p)
	{
		return m_pImpl->GetFile(p);
	}
	std::vector< std::pair<std::filesystem::path, ContentType> > NZBDrive::GetContent()
	{
		return m_pImpl->GetContent();
	}
	void NZBDrive::EnumFiles(std::function<void(const std::filesystem::path& path, std::shared_ptr<IFile> file)> callback)
	{
		m_pImpl->EnumFiles(callback);
	}
	uint_fast64_t NZBDrive::RXBytes()
	{
		return m_pImpl->RXBytes();
	}
	uint_fast64_t NZBDrive::GetTotalNumberOfBytes() const
	{
		return m_pImpl->GetTotalNumberOfBytes();
	}



}
