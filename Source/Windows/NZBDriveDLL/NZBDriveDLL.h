#pragma once

using namespace System;

class NZBDrive;
class NZBDokanDrive;

namespace NZBDriveDLL 
{
	public enum class LogLevelType
	{
		Debug,
		Info,
		PopupInfo,
		Warning,
		Error,
		PopupError,
		Fatal
	};

	public value struct UsenetServer
	{
		String^ ServerName;
		String^ UserName;
		String^ Password;
		String^ Port;
		bool UseSSL;
		int32_t Connections;
		int32_t Priority;
		int32_t Timeout;
		int32_t Pipeline;
	};

	public enum class NetworkMode
	{ 
		Constant, 
		Adaptive 
	};

	public value struct NetworkThrottling
	{
		NetworkMode Mode;
		int32_t NetworkLimit;
		int32_t FastPreCache;
		int32_t AdaptiveIORatioPCT;
		int32_t BackgroundNetworkRate;
	};

	public enum class ConnectionState { Disconnected, Connecting, Idle, Working };
	public enum class SegmentState { None, Loading, HasData, MissingSegment, DownloadFailed };

	public delegate void MountStatusFunction(const int32_t nzbID, const int32_t parts, const int32_t total);
	public delegate void NZBFileOpenFunction(const int32_t nzbID, const String^ path);
	public delegate void NZBFileCloseFunction(const int32_t nzbID);
	public delegate void FileAddedFunction(const int32_t nzbID, const int32_t fileID, const int32_t segments, const uint64_t size);
	public delegate void FileInfoFunction(const int32_t fileID, const String^ name, const uint64_t size);
	public delegate void FileSegmentStateChangedFunction(const int32_t fileID, const int32_t segment, const SegmentState state);
	public delegate void FileRemovedFunction(const int32_t fileID);
	public delegate void ConnectionStateChangedFunction(const ConnectionState state, const int32_t server, const int32_t thread);
	public delegate void EventLogFunction(const LogLevelType lvl, const String^ msg);
	public delegate void ConnectionInfoFunction(const int64_t time, const int32_t server, const uint64_t bytesTX, uint64_t bytesRX, 
		uint32_t missingSegmentCount, uint32_t connectionTimeoutCount, uint32_t errorCount);


	public ref class NZBDrive
	{
	private:
		::NZBDokanDrive* m_pimpl;

	internal: void NotifyNZBFileOpen(const int32_t nzbID, const String^ path);
	internal: void NotifyNZBFileClose(const int32_t nzbID);
	internal: void NotifyFileAdded(const int32_t nzbID, const int32_t fileID, const int32_t segments, const uint64_t size);
	internal: void NotifyFileInfo(const int32_t fileID, const String^ name, const uint64_t size);
	internal: void NotifyFileSegmentStateChanged(const int32_t fileID, const int32_t segment, const SegmentState state);
	internal: void NotifyFileRemoved(const int32_t fileID);
	internal: void NotifyConnectionStateChanged(const ConnectionState state, const int32_t server, const int32_t thread);
	internal: void NotifyEventLog(const LogLevelType lvl, const String^ msg);
	internal: void NotifyConnectionInfo(const int64_t time, const int32_t server, const uint64_t bytesTX, uint64_t bytesRX, 
		uint32_t missingSegmentCount, uint32_t connectionTimeoutCount, uint32_t errorCount);

	public:

		NZBDrive();
		~NZBDrive();
		!NZBDrive();
		int32_t Start();
		int32_t Stop();
		void RemoveServer(const int32_t id);
		int32_t AddServer(const UsenetServer server);
		int32_t Mount(String^ nzbfile, MountStatusFunction^ callback);
		int32_t Unmount(String^ nzbfile);
		property String^ DriveLetter { String^ get(); void set(String^ settings); }
		property String^ CachePath { String^ get(); void set(String^ settings); }
		property bool PreCheckSegments { bool get(); void set(bool settings); }
		property NetworkThrottling Throttling { void set(NetworkThrottling val); }
		property uint_fast64_t RXBytes { uint_fast64_t get(); }
		property LogLevelType LogLevel { LogLevelType get(); void set(LogLevelType level); }

		event NZBFileOpenFunction^ NZBFileOpen;
		event NZBFileCloseFunction^ NZBFileClose;
		event FileAddedFunction^ FileAdded;
		event FileInfoFunction^ FileInfo;
		event FileSegmentStateChangedFunction^ FileSegmentStateChanged;
		event FileRemovedFunction^ FileRemoved;
		event ConnectionStateChangedFunction^ ConnectionStateChanged;
		event EventLogFunction^ EventLog;
		event ConnectionInfoFunction^ ConnectionInfo;
	};
}
