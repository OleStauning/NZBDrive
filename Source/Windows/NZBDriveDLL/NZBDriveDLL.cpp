// This is the main DLL file.

#include "stdafx.h"
#include "NZBDriveDLL.h"

#ifdef __cplusplus_cli // FIX: generic clash in C++/CLI and boost::filesystem.
#define generic __identifier(generic)// FIX: generic clash in C++/CLI and boost::filesystem.
#endif

#include "NZBDokanDrive.hpp"

#ifdef __cplusplus_cli// FIX: generic clash in C++/CLI and boost::filesystem.
#undef generic// FIX: generic clash in C++/CLI and boost::filesystem.
#endif// FIX: generic clash in C++/CLI and boost::filesystem.

#include <boost/bind.hpp>
#include <vcclr.h>

using namespace System;
using namespace System::Runtime::InteropServices;

namespace NZBDriveDLL 
{

	namespace
	{
		void except_convert()
		{
			try { throw; }
			catch (const std::exception& err)
			{
				throw gcnew System::Exception(gcnew System::String(err.what()));
			}
		}

		LogLevelType adapt(NZBDokanDrive::LogLevel level)
		{
			switch(level)
			{
			case NZBDokanDrive::Debug: return LogLevelType::Debug;
			case NZBDokanDrive::Info: return LogLevelType::Info;
			case NZBDokanDrive::PopupInfo: return LogLevelType::PopupInfo;
			case NZBDokanDrive::Warning: return LogLevelType::Warning;
			case NZBDokanDrive::Error: return LogLevelType::Error;
			case NZBDokanDrive::PopupError: return LogLevelType::PopupError;
			case NZBDokanDrive::Fatal: return LogLevelType::Fatal;
			}
			throw std::exception("Internal error");
		}

		NZBDokanDrive::LogLevel adapt(LogLevelType level)
		{
			switch(level)
			{
			case LogLevelType::Debug: return NZBDokanDrive::Debug;
			case LogLevelType::Info: return NZBDokanDrive::Info;
			case LogLevelType::PopupInfo: return NZBDokanDrive::PopupInfo;
			case LogLevelType::Warning: return NZBDokanDrive::Warning;
			case LogLevelType::Error: return NZBDokanDrive::Error;
			case LogLevelType::PopupError: return NZBDokanDrive::PopupError;
			case LogLevelType::Fatal: return NZBDokanDrive::Fatal;
			}
			throw std::exception("Internal error");
		}

		NZBDokanDrive::NetworkThrottling::NetworkMode adapt(const NetworkMode val)
		{
			switch (val)
			{
			case NetworkMode::Adaptive: return NZBDokanDrive::NetworkThrottling::NetworkMode::Adaptive;
			case NetworkMode::Constant: return NZBDokanDrive::NetworkThrottling::NetworkMode::Constant;
			}
			throw std::exception("Internal error");
		}

		NZBDokanDrive::NetworkThrottling adapt(const NetworkThrottling& val)
		{
			NZBDokanDrive::NetworkThrottling res;
			res.Mode = adapt(val.Mode);
			res.NetworkLimit = val.NetworkLimit;
			res.FastPreCache = val.FastPreCache;
			res.AdaptiveIORatioPCT = val.AdaptiveIORatioPCT;
			res.BackgroundNetworkRate = val.BackgroundNetworkRate;
			return res;
		}

		String^ adapt(const boost::filesystem::path& str)
		{
			return gcnew String(str.string().c_str());
		}

		String^ adapt(const std::string& str)
		{
			return gcnew String(str.c_str());
		}

		NZBDriveDLL::SegmentState adapt(const NZBDokanDrive::SegmentState state)
		{
			switch (state)
			{
			case NZBDokanDrive::SegmentState::None: return NZBDriveDLL::SegmentState::None;
			case NZBDokanDrive::SegmentState::Loading: return NZBDriveDLL::SegmentState::Loading;
			case NZBDokanDrive::SegmentState::HasData: return NZBDriveDLL::SegmentState::HasData;
			case NZBDokanDrive::SegmentState::MissingSegment: return NZBDriveDLL::SegmentState::MissingSegment;
			case NZBDokanDrive::SegmentState::DownloadFailed: return NZBDriveDLL::SegmentState::DownloadFailed;
			}
			throw std::exception("Internal error");
		}

		NZBDriveDLL::ConnectionState adapt(const NZBDokanDrive::ConnectionState state)
		{
			switch (state)
			{
			case NZBDokanDrive::ConnectionState::Connecting: return NZBDriveDLL::ConnectionState::Connecting;
			case NZBDokanDrive::ConnectionState::Disconnected: return NZBDriveDLL::ConnectionState::Disconnected;
			case NZBDokanDrive::ConnectionState::Idle: return NZBDriveDLL::ConnectionState::Idle;
			case NZBDokanDrive::ConnectionState::Working: return NZBDriveDLL::ConnectionState::Working;
			}
			throw std::exception("Internal error");
		}
	}

	void NZBDrive::NotifyNZBFileOpen(const int32_t nzbID, const String^ path)
	{
		NZBFileOpen(nzbID, path);
	}
	void NotifyNZBFileOpenProxy(gcroot<NZBDrive^> this_, const int32_t nzbID, const boost::filesystem::path& path)
	{
		this_->NotifyNZBFileOpen(nzbID, adapt(path));
	}
	void NZBDrive::NotifyNZBFileClose(const int32_t nzbID)
	{
		NZBFileClose(nzbID);
	}
	void NotifyNZBFileCloseProxy(gcroot<NZBDrive^> this_, const int32_t nzbID)
	{
		this_->NotifyNZBFileClose(nzbID);
	}
	void NZBDrive::NotifyFileAdded(const int32_t nzbID, const int32_t fileID, const int32_t segments, const uint64_t size)
	{
		FileAdded(nzbID, fileID, segments, size);
	}
	void NotifyFileAddedProxy(gcroot<NZBDrive^> this_, const int32_t nzbID, const int32_t fileID, const int32_t segments, const uint64_t size)
	{
		this_->NotifyFileAdded(nzbID,fileID,segments,size);
	}
	void NZBDrive::NotifyFileInfo(const int32_t fileID, const String^ name, const uint64_t size)
	{
		FileInfo(fileID, name, size);
	}
	void NotifyFileInfoProxy(gcroot<NZBDrive^> this_, const int32_t fileID, const boost::filesystem::path& name, const uint64_t size)
	{
		this_->NotifyFileInfo(fileID, adapt(name), size);
	}
	void NZBDrive::NotifyFileSegmentStateChanged(const int32_t fileID, const int32_t segment, const SegmentState state)
	{
		FileSegmentStateChanged(fileID, segment, state);
	}
	void NotifyFileSegmentStateChangedProxy(gcroot<NZBDrive^> this_, const int32_t fileID, const int32_t segment, const NZBDokanDrive::SegmentState state)
	{
		this_->NotifyFileSegmentStateChanged(fileID,segment,adapt(state));
	}
	void NZBDrive::NotifyFileRemoved(const int32_t fileID)
	{
		FileRemoved(fileID);
	}
	void NotifyFileRemovedProxy(gcroot<NZBDrive^> this_, const int32_t fileID)
	{
		this_->NotifyFileRemoved(fileID);
	}
	void NZBDrive::NotifyConnectionStateChanged(const ConnectionState state, const int32_t server, const int32_t thread)
	{
		ConnectionStateChanged(state, server, thread);
	}
	void NotifyConnectionStateChangedProxy(gcroot<NZBDrive^> this_, const NZBDokanDrive::ConnectionState state, const int32_t server, const int32_t thread)
	{
		this_->NotifyConnectionStateChanged(adapt(state), server, thread);
	}
	void NZBDrive::NotifyEventLog(const LogLevelType lvl, const String^ msg)
	{
		EventLog(lvl, msg);
	}
	void NotifyEventLogProxy(gcroot<NZBDrive^> this_, const NZBDokanDrive::LogLevel lvl,const std::string& msg)
	{
		this_->NotifyEventLog(adapt(lvl), adapt(msg));
	}
	void NZBDrive::NotifyConnectionInfo(const int64_t time, const int32_t server, const uint64_t bytesTX, uint64_t bytesRX, 
		uint32_t missingSegmentCount, uint32_t connectionTimeoutCount, uint32_t errorCount)
	{
		ConnectionInfo(time, server, bytesTX, bytesRX, missingSegmentCount, connectionTimeoutCount, errorCount);
	}
	void NotifyConnectionInfoProxy(gcroot<NZBDrive^> this_, const int64_t time, const int32_t server, const uint64_t bytesTX, uint64_t bytesRX, 
		uint32_t missingSegmentCount, uint32_t connectionTimeoutCount, uint32_t errorCount)
	{
		this_->NotifyConnectionInfo(time, server, bytesTX, bytesRX, missingSegmentCount, connectionTimeoutCount, errorCount);
	}


	private ref class Servers_Impl 
	{
	private:
		::NZBDokanDrive& m_rimpl;
	public:
		Servers_Impl(::NZBDokanDrive* pimpl):m_rimpl(*pimpl){}
	};

	NZBDrive::NZBDrive()
	{
		m_pimpl=new ::NZBDokanDrive();

		m_pimpl->SetNZBFileOpenHandler(boost::bind(NotifyNZBFileOpenProxy, gcroot<NZBDrive^>(this), _1, _2));
		m_pimpl->SetNZBFileCloseHandler(boost::bind(NotifyNZBFileCloseProxy, gcroot<NZBDrive^>(this), _1));
		m_pimpl->SetFileAddedHandler(boost::bind(NotifyFileAddedProxy, gcroot<NZBDrive^>(this), _1, _2, _3, _4));
		m_pimpl->SetFileInfoHandler(boost::bind(NotifyFileInfoProxy, gcroot<NZBDrive^>(this), _1, _2, _3));
		m_pimpl->SetFileSegmentStateChangedHandler(boost::bind(NotifyFileSegmentStateChangedProxy, gcroot<NZBDrive^>(this), _1, _2, _3));
		m_pimpl->SetFileRemovedHandler(boost::bind(NotifyFileRemovedProxy, gcroot<NZBDrive^>(this), _1));
		m_pimpl->SetConnectionStateChangedHandler(boost::bind(NotifyConnectionStateChangedProxy, gcroot<NZBDrive^>(this), _1, _2, _3));
		m_pimpl->SetEventLogHandler(boost::bind(NotifyEventLogProxy, gcroot<NZBDrive^>(this), _1, _2));
		m_pimpl->SetConnectionInfoHandler(boost::bind(NotifyConnectionInfoProxy, gcroot<NZBDrive^>(this), _1, _2, _3, _4, _5, _6, _7));
	}

	NZBDrive::~NZBDrive()
	{
		this->!NZBDrive();
	}
	NZBDrive::!NZBDrive()
	{
		if (m_pimpl)
		{
			m_pimpl->Stop();
			m_pimpl->SetNZBFileOpenHandler(nullptr);
			m_pimpl->SetNZBFileCloseHandler(nullptr);
			m_pimpl->SetFileAddedHandler(nullptr);
			m_pimpl->SetFileInfoHandler(nullptr);
			m_pimpl->SetFileSegmentStateChangedHandler(nullptr);
			m_pimpl->SetFileRemovedHandler(nullptr);
			m_pimpl->SetConnectionStateChangedHandler(nullptr);
			m_pimpl->SetEventLogHandler(nullptr);
			m_pimpl->SetConnectionInfoHandler(nullptr);

			delete m_pimpl;
			m_pimpl = 0;
		}
	}
	int32_t NZBDrive::Start()
	{
		try
		{
			return (int)m_pimpl->Start();
		}
		catch(...)
		{
			except_convert();
		}
		return 0;
	}
	int32_t NZBDrive::Stop()
	{
		try
		{
			return (int)m_pimpl->Stop();
		}
		catch(...)
		{
			except_convert();
		}
		return 0;
	}

	void MountStatusNotifyProxy(gcroot<MountStatusFunction^> callback, const int32_t nzbID, const int32_t parts, const int32_t total)
	{ 
		for each(MountStatusFunction^ handle in callback->GetInvocationList())
        {
            handle->Invoke(nzbID,parts,total);
        }
	}

	int32_t NZBDrive::AddServer(const UsenetServer val)
	{
		try
		{
			NZBDokanDrive::UsenetServer unmval;
			unmval.ServerName = (char*)Marshal::StringToHGlobalAnsi(val.ServerName).ToPointer();
			unmval.UserName = (char*)Marshal::StringToHGlobalAnsi(val.UserName).ToPointer();
			unmval.Password = (char*)Marshal::StringToHGlobalAnsi(val.Password).ToPointer();
			unmval.Port = (char*)Marshal::StringToHGlobalAnsi(val.Port).ToPointer();
			unmval.UseSSL = val.UseSSL;
			unmval.Connections = val.Connections;
			unmval.Priority = val.Priority;
			unmval.Timeout = val.Timeout;
			unmval.Pipeline = val.Pipeline;
			return m_pimpl->AddServer(unmval);
		}
		catch (...)
		{
			except_convert();
		}
		return -1;
	}

	void NZBDrive::RemoveServer(const int32_t id)
	{
		try
		{
			m_pimpl->RemoveServer(id);
		}
		catch (...)
		{
			except_convert();
		}
	}

	int32_t NZBDrive::Mount(String^ nzbfile, MountStatusFunction^ callback)
	{
		wchar_t* wnzbfile = (wchar_t*)Marshal::StringToHGlobalUni(nzbfile).ToPointer(); 
		try
		{
			return (int)m_pimpl->Mount(wnzbfile, boost::bind(MountStatusNotifyProxy,gcroot<MountStatusFunction^>(callback),_1,_2,_3));
		}
		catch(...)
		{
			except_convert();
		}
		finally
		{
			Marshal::FreeHGlobal((IntPtr)wnzbfile);
		}
		throw gcnew System::Exception(gcnew System::String("Internal error"));
	}
	int32_t NZBDrive::Unmount(String^ nzbfile)
	{
		wchar_t* wnzbfile = (wchar_t*)Marshal::StringToHGlobalUni(nzbfile).ToPointer(); 
		try
		{
			return (int)m_pimpl->Unmount(wnzbfile);
		}
		catch(...)
		{
			except_convert();
		}
		finally
		{
			Marshal::FreeHGlobal((IntPtr)wnzbfile);
		}
		throw gcnew System::Exception(gcnew System::String("Internal error"));
	}

	void NZBDrive::DriveLetter::set(String^ val)
	{
		wchar_t* myval = (wchar_t*)Marshal::StringToHGlobalUni(val).ToPointer(); 
		try
		{
			m_pimpl->SetDriveLetter(myval);
		}
		catch(...)
		{
			except_convert();
		}
		finally
		{
			Marshal::FreeHGlobal((IntPtr)myval);
		}
	}
	String^ NZBDrive::DriveLetter::get()
	{
		try
		{
			return gcnew String(m_pimpl->GetDriveLetter().c_str());
		}
		catch(...)
		{
			except_convert();
		}
		throw gcnew System::Exception(gcnew System::String("Internal error"));
	}


	//property UsenetServer Server[int] { UsenetServer get(int i); void set(int i, UsenetServer s); }

	void NZBDrive::CachePath::set(String^ val)
	{
		char* myval = (char*)Marshal::StringToHGlobalAnsi(val).ToPointer(); 
		try
		{
			m_pimpl->SetCachePath(myval);
		}
		catch(...)
		{
			except_convert();
		}
		finally
		{
			Marshal::FreeHGlobal((IntPtr)myval);
		}
	}
	String^ NZBDrive::CachePath::get()
	{
		try
		{
			return gcnew String(m_pimpl->GetCachePath().c_str());
		}
		catch(...)
		{
			except_convert();
		}
		throw gcnew System::Exception(gcnew System::String("Internal error"));
	}
	void NZBDrive::PreCheckSegments::set(bool val)
	{
		try
		{
			m_pimpl->SetPreCheckSegments(val);
		}
		catch(...)
		{
			except_convert();
		}
	}
	void NZBDrive::Throttling::set(NetworkThrottling val)
	{
		try
		{
			m_pimpl->SetNetworkThrottling(adapt(val));
		}
		catch (...)
		{
			except_convert();
		}
	}

	bool NZBDrive::PreCheckSegments::get()
	{
		try
		{
			return m_pimpl->GetPreCheckSegments();
		}
		catch(...)
		{
			except_convert();
		}
		throw gcnew System::Exception(gcnew System::String("Internal error"));
	}

	uint_fast64_t NZBDrive::RXBytes::get()
	{
		try
		{
			return m_pimpl->GetRXBytes();
		}
		catch(...)
		{
			except_convert();
		}
		throw gcnew System::Exception(gcnew System::String("Internal error"));
	}

	LogLevelType NZBDrive::LogLevel::get()
	{
		try
		{
			return adapt(m_pimpl->GetLogLevel());
		}
		catch(...)
		{
			except_convert();
		}
		throw gcnew System::Exception(gcnew System::String("Internal error"));
	}
	void NZBDrive::LogLevel::set(LogLevelType level)
	{
		try
		{
			m_pimpl->SetLogLevel(adapt(level));
		}
		catch(...)
		{
			except_convert();
		}
	}


}