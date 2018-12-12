/*
	Copyright (c) 2017, Ole Stauning
	All rights reserved.

	Use of this source code is governed by a GPLv3-style license that can be found
	in the LICENSE file.
*/

#include "NZBDokanDrive.hpp"
#include "IDirectory.hpp"
#include "Logger.hpp"
#include <aclapi.h>
#include "fileinfo.h"
#include "NZBFuseDrive.hpp"
#include "text_tool.hpp"
#include <memory>
#include <unordered_map>
#include <mutex>
#include <future>
#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <thread>


namespace
{

	template <typename T, typename... TArgs>
	static void SendEvent(T& handler, TArgs&&... args)
	{
		auto h = handler;
		if (h) h(std::forward<TArgs>(args)...);
	}

	template <typename T> T adapt(const T& val) { return val; }

	NZBDokanDrive::LogLevel adapt(ByteFountain::LogLineLevel level)
	{
		switch(level)
		{
		case ByteFountain::LogLineLevel::Debug: return NZBDokanDrive::LogLevel::Debug;
		case ByteFountain::LogLineLevel::Info: return NZBDokanDrive::LogLevel::Info;
		case ByteFountain::LogLineLevel::PopupInfo: return NZBDokanDrive::LogLevel::PopupInfo;
		case ByteFountain::LogLineLevel::Warning: return NZBDokanDrive::LogLevel::Warning;
		case ByteFountain::LogLineLevel::Error: return NZBDokanDrive::LogLevel::Error;
		case ByteFountain::LogLineLevel::PopupError: return NZBDokanDrive::LogLevel::PopupError;
		case ByteFountain::LogLineLevel::Fatal: return NZBDokanDrive::LogLevel::Fatal;
		}
		throw std::exception("Internal error");		
	}

	ByteFountain::LogLineLevel adapt(NZBDokanDrive::LogLevel level)
	{
		switch(level)
		{
		case NZBDokanDrive::LogLevel::Debug: return ByteFountain::LogLineLevel::Debug;
		case NZBDokanDrive::LogLevel::Info: return ByteFountain::LogLineLevel::Info;
		case NZBDokanDrive::LogLevel::PopupInfo: return ByteFountain::LogLineLevel::PopupInfo;
		case NZBDokanDrive::LogLevel::Warning: return ByteFountain::LogLineLevel::Warning;
		case NZBDokanDrive::LogLevel::Error: return ByteFountain::LogLineLevel::Error;
		case NZBDokanDrive::LogLevel::PopupError: return ByteFountain::LogLineLevel::PopupError;
		case NZBDokanDrive::LogLevel::Fatal: return ByteFountain::LogLineLevel::Fatal;
		}
		throw std::exception("Internal error");		
	}

	NZBDokanDrive::ConnectionState adapt(const ByteFountain::ConnectionState& state)
	{
		switch (state)
		{
		case ByteFountain::ConnectionState::Disconnected: return NZBDokanDrive::Disconnected;
		case ByteFountain::ConnectionState::Connecting: return NZBDokanDrive::Connecting;
		case ByteFountain::ConnectionState::Idle: return NZBDokanDrive::Idle;
		case ByteFountain::ConnectionState::Working: return NZBDokanDrive::Working;
		}
		throw std::exception("Internal error");
	}

	NZBDokanDrive::SegmentState adapt(const ByteFountain::SegmentState& state)
	{
		switch (state)
		{
		case ByteFountain::SegmentState::None: return NZBDokanDrive::None;
		case ByteFountain::SegmentState::Loading: return NZBDokanDrive::Loading;
		case ByteFountain::SegmentState::HasData: return NZBDokanDrive::HasData;
		case ByteFountain::SegmentState::MissingSegment: return NZBDokanDrive::MissingSegment;
		case ByteFountain::SegmentState::DownloadFailed: return NZBDokanDrive::DownloadFailed;
		}
		throw std::exception("Internal error");
	}

	bool IsDrive(const std::wstring& drive)
	{
		#define BUFSIZE 512

		WCHAR szTemp[BUFSIZE];
		szTemp[0] = L'\0';
		size_t slen;

		if ((slen = GetLogicalDriveStrings(BUFSIZE - 1, szTemp)) > 0)
		{
			std::wstring drives(szTemp, slen);
			std::size_t found = drives.find(drive);
			if (found != std::wstring::npos) return true;
		}

		return  false;
	}

}


class NZBDokanDrive_IMPL
{
	NZBDokanDrive::MountStatusFunction m_mountStatusFunction;
	NZBDokanDrive::EventLogFunction m_eventLogFunction;

	ByteFountain::Logger m_logger;

	std::shared_ptr<ByteFountain::NZBFuseDrive> m_NZBFuseDrive;

	enum Status { Stopped, StartingDrive, StartingDokan, Started, StoppingDokan, StoppingDrive };

	Status m_status;
	std::mutex m_statusMutex;

	std::thread m_thread;

	//settings:
	std::wstring m_drive_letter;
	bool m_pre_check_segments;
	ByteFountain::NetworkThrottling m_throttling;;

	WCHAR m_mount_point[MAX_PATH];

public:
#pragma warning ( disable : 4351 )
	NZBDokanDrive_IMPL() :
		m_logger([this](const ByteFountain::LogLineLevel lvl, const std::string& msg)
	{
		SendEvent(m_eventLogFunction, adapt(lvl), msg);
	}
		),
		m_NZBFuseDrive(new ByteFountain::NZBFuseDrive(m_logger)),
		m_status(Stopped),
		m_thread(),
		m_drive_letter(L"N"),
		m_pre_check_segments(false),
		m_mount_point()
	{
	}

	ByteFountain::Logger& Logger()
	{
		return m_logger;
	}

#pragma warning ( default : 4351 )

	~NZBDokanDrive_IMPL()
	{
		m_eventLogFunction = nullptr;
		m_NZBFuseDrive.reset();
	}

	void OnEventLog(const ByteFountain::LogLineLevel lvl, const std::string& message)
	{
		SendEvent(m_eventLogFunction, adapt(lvl), message);
	}

	NZBDokanDrive::Result Start()
	{
		std::unique_lock<std::mutex> lk(m_statusMutex);

		m_NZBFuseDrive->SetNetworkThrottling(m_throttling);

		auto x = (ByteFountain::INZBDrive*)m_NZBFuseDrive.get();

		if (!x->Start()) return NZBDokanDrive::DriveNotStarted;

		if (!_SetDriveLetter(m_drive_letter)) return NZBDokanDrive::DriveNotStarted;

		return NZBDokanDrive::Success;
	}

	NZBDokanDrive::Result Stop(Status reason = Stopped)
	{
		std::unique_lock<std::mutex> lk(m_statusMutex);

		if (m_status == Stopped) return NZBDokanDrive::Success;

//		_SetDriveLetter(L"");

		m_status = StoppingDrive;
		m_NZBFuseDrive->Stop();
		m_status = reason;

		return NZBDokanDrive::Result::Success;
	}

	std::wstring GetDriveLetter() { return m_drive_letter; }
	bool _SetDriveLetter(const std::wstring& val)
	{
		std::filesystem::path wmount_point = val + L":\\";

//		if (IsDrive(wmount_point)) return false;

		m_NZBFuseDrive->SetDrivePath(wmount_point.string());
		/*
		if (!IsDrive(wmount_point))
		{
			m_status = Stopped;
			return false;
		}
		*/
//		m_status = Started;

		return true;
	}


	bool SetDriveLetter(const std::wstring& val)
	{
		std::unique_lock<std::mutex> lk(m_statusMutex);
		return _SetDriveLetter(val);
	}

	std::filesystem::path MountDir(std::filesystem::path path)
	{
		static const std::string invalid_chars = ":\\/";
		auto s = path.string();
		std::replace_if(s.begin(), s.end(),
			[](char c) { return invalid_chars.find(c) != std::string::npos; }, '_');
		return std::filesystem::path(s);
	};

	int32_t Mount(const wchar_t* nzbpath, NZBDokanDrive::MountStatusFunction callback)
	{
		std::filesystem::path mynzbmount(nzbpath);
		std::filesystem::path mynzbpath(nzbpath);

		return m_NZBFuseDrive->Mount(MountDir(mynzbmount), mynzbpath.string(),
			[callback](const int32_t nzbID, const int32_t parts, const int32_t total)
		{
			callback(nzbID, parts, total);
		});
	}
	int32_t Unmount(const wchar_t* nzbpath)
	{
		std::filesystem::path mynzbmount(nzbpath);
		return m_NZBFuseDrive->Unmount(MountDir(mynzbmount));
	}

	int32_t AddServer(const NZBDokanDrive::UsenetServer& server)
	{
		ByteFountain::UsenetServer srv;
		srv.ServerName = server.ServerName;
		srv.Port = server.Port;
		srv.UserName = server.UserName;
		srv.Password = server.Password;
		srv.Connections = server.Connections;
		srv.UseSSL = server.UseSSL;
		srv.Priority = server.Priority;
		srv.Timeout = server.Timeout;
		srv.Pipeline = server.Pipeline;
		return m_NZBFuseDrive->AddServer(srv);
	}

	void RemoveServer(const int32_t id)
	{
		m_NZBFuseDrive->RemoveServer(id);
	}

	bool GetPreCheckSegments() { return m_pre_check_segments; }
	void SetPreCheckSegments(const bool val) { m_pre_check_segments = val; }
	void SetNetworkThrottling(const NZBDokanDrive::NetworkThrottling& val)
	{
		switch (val.Mode)
		{
		case NZBDokanDrive::NetworkThrottling::Adaptive: m_throttling.Mode = ByteFountain::NetworkThrottling::Adaptive; break;
		case NZBDokanDrive::NetworkThrottling::Constant: m_throttling.Mode = ByteFountain::NetworkThrottling::Constant; break;
		default: m_throttling.Mode = ByteFountain::NetworkThrottling::Constant; break;
		}
		m_throttling.NetworkLimit = val.NetworkLimit;
		m_throttling.FastPreCache = val.FastPreCache;
		m_throttling.AdaptiveIORatioPCT = val.AdaptiveIORatioPCT;
		m_throttling.BackgroundNetworkRate = val.BackgroundNetworkRate;

		m_NZBFuseDrive->SetNetworkThrottling(m_throttling);
	}
	uint_fast64_t GetRXBytes() { return m_NZBFuseDrive->RXBytes(); }

	void SetMountStatusHandler(NZBDokanDrive::MountStatusFunction handler)
	{
		m_mountStatusFunction = handler;
	}

	template <typename... TArgs>
	std::function<void(TArgs...)> RelayEvent(std::function<void(TArgs...)>& handler)
	{
		return [handler](TArgs&&... args)
		{
			auto h = handler;
			if (h) handler(std::forward<TArgs>(args)...);
		};
	}

	void SetNZBFileOpenHandler(NZBDokanDrive::NZBFileOpenFunction handler)
	{
		m_NZBFuseDrive->SetNZBFileOpenHandler(RelayEvent(handler));
	}
	void SetNZBFileCloseHandler(NZBDokanDrive::NZBFileCloseFunction handler)
	{
		m_NZBFuseDrive->SetNZBFileCloseHandler(RelayEvent(handler));
	}
	void SetFileAddedHandler(NZBDokanDrive::FileAddedFunction handler)
	{
		m_NZBFuseDrive->SetFileAddedHandler(RelayEvent(handler));
	}
	void SetFileInfoHandler(NZBDokanDrive::FileInfoFunction handler)
	{
		m_NZBFuseDrive->SetFileInfoHandler(RelayEvent(handler));
	}
	void SetFileSegmentStateChangedHandler(NZBDokanDrive::FileSegmentStateChangedFunction handler)
	{
		m_NZBFuseDrive->SetFileSegmentStateChangedHandler([handler](const int32_t fileID, const int32_t segment, const ByteFountain::SegmentState state)
		{
			SendEvent(handler, fileID, segment, adapt(state));
		});
	}
	void SetFileRemovedHandler(NZBDokanDrive::FileRemovedFunction handler)
	{
		m_NZBFuseDrive->SetFileRemovedHandler(RelayEvent(handler));
	}
	void SetConnectionStateChangedHandler(NZBDokanDrive::ConnectionStateChangedFunction handler)
	{
		m_NZBFuseDrive->SetConnectionStateChangedHandler([handler](const ByteFountain::ConnectionState state, const int32_t server, const int32_t thread)
		{
			SendEvent(handler, adapt(state), server, thread);
		});
	}
	void SetConnectionInfoHandler(NZBDokanDrive::ConnectionInfoFunction handler)
	{
		m_NZBFuseDrive->SetConnectionInfoHandler(RelayEvent(handler));
	}
	void SetEventLogHandler(NZBDokanDrive::EventLogFunction handler)
	{
		m_eventLogFunction = handler;
	}

	NZBDokanDrive::LogLevel GetLogLevel()
	{
		return adapt(m_logger.GetLevel());
	}

	void SetLogLevel(NZBDokanDrive::LogLevel level)
	{
		m_logger.SetLevel(adapt(level));
	}
};

NZBDokanDrive::NZBDokanDrive():m_pimpl(new NZBDokanDrive_IMPL())
{	
}
NZBDokanDrive::~NZBDokanDrive()
{
	delete m_pimpl;
}

NZBDokanDrive::Result NZBDokanDrive::Start()
{
	return m_pimpl->Start();
}

NZBDokanDrive::Result NZBDokanDrive::Stop()
{
	return m_pimpl->Stop();
}

int32_t NZBDokanDrive::AddServer(const UsenetServer& server)
{
	return m_pimpl->AddServer(server);
}
void NZBDokanDrive::RemoveServer(const int32_t id)
{
	m_pimpl->RemoveServer(id);
}

int32_t NZBDokanDrive::Mount(const wchar_t* nzbpath, NZBDokanDrive::MountStatusFunction callback)
{
	return m_pimpl->Mount(nzbpath,callback);
}
int32_t NZBDokanDrive::Unmount(const wchar_t* nzbpath)
{
	return m_pimpl->Unmount(nzbpath);
}
std::wstring NZBDokanDrive::GetDriveLetter()
{
	return m_pimpl->GetDriveLetter();
}
void NZBDokanDrive::SetDriveLetter(const std::wstring& val)
{
	m_pimpl->SetDriveLetter(val);
}

bool NZBDokanDrive::GetPreCheckSegments()
{
	return m_pimpl->GetPreCheckSegments();
}
void NZBDokanDrive::SetPreCheckSegments(const bool val)
{
	m_pimpl->SetPreCheckSegments(val);
}
void NZBDokanDrive::SetNetworkThrottling(const NetworkThrottling& val)
{
	m_pimpl->SetNetworkThrottling(val);
}

void NZBDokanDrive::SetMountStatusHandler(MountStatusFunction handler)
{
	m_pimpl->SetMountStatusHandler(handler);
}
void NZBDokanDrive::SetNZBFileOpenHandler(NZBFileOpenFunction handler)
{
	m_pimpl->SetNZBFileOpenHandler(handler);
}
void NZBDokanDrive::SetNZBFileCloseHandler(NZBFileCloseFunction handler)
{
	m_pimpl->SetNZBFileCloseHandler(handler);
}
void NZBDokanDrive::SetFileAddedHandler(FileAddedFunction handler)
{
	m_pimpl->SetFileAddedHandler(handler);
}
void NZBDokanDrive::SetFileInfoHandler(FileInfoFunction handler)
{
	m_pimpl->SetFileInfoHandler(handler);
}
void NZBDokanDrive::SetFileSegmentStateChangedHandler(FileSegmentStateChangedFunction handler)
{
	m_pimpl->SetFileSegmentStateChangedHandler(handler);
}
void NZBDokanDrive::SetFileRemovedHandler(FileRemovedFunction handler)
{
	m_pimpl->SetFileRemovedHandler(handler);
}
void NZBDokanDrive::SetConnectionStateChangedHandler(ConnectionStateChangedFunction handler)
{
	m_pimpl->SetConnectionStateChangedHandler(handler);
}
void NZBDokanDrive::SetConnectionInfoHandler(ConnectionInfoFunction handler)
{
	m_pimpl->SetConnectionInfoHandler(handler);
}
void NZBDokanDrive::SetEventLogHandler(EventLogFunction handler)
{
	m_pimpl->SetEventLogHandler(handler);
}

uint_fast64_t NZBDokanDrive::GetRXBytes()
{
	return m_pimpl->GetRXBytes();
}
NZBDokanDrive::LogLevel NZBDokanDrive::GetLogLevel()
{
	return m_pimpl->GetLogLevel();
}
void NZBDokanDrive::SetLogLevel(NZBDokanDrive::LogLevel level)
{
	m_pimpl->SetLogLevel(level);
}
