#include <boost/filesystem.hpp>
#include <functional>
#include <string>


class NZBDokanDrive_IMPL;


class NZBDokanDrive
{
	NZBDokanDrive_IMPL* m_pimpl;

public:

	enum Result
	{
		Success=0,
		MountDokanError = -100,
		MountDriveLetterError = -102,
		MountDriverInstallError = -103,
		MountStartError = -104,
		MountError = -105,
		MountPointError = -106,
		TrialTimeout = -107,
		DriveNotStarted = -108
	};

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
	};

	enum LogLevel
	{
		Debug,
		Info,
		PopupInfo,
		Warning,
		Error,
		PopupError,
		Fatal
	};

	struct NewsConnectionEvent
	{
		enum What
		{
			Disconnected, Connecting, Idle, Working
		};
		What what;
		int32_t server;
		int32_t thread;
	};


	struct NetworkThrottling
	{
		enum NetworkMode { Constant, Adaptive } Mode = Adaptive;

		std::size_t NetworkLimit;
		std::size_t FastPreCache;
		std::size_t AdaptiveIORatioPCT;
		std::size_t BackgroundNetworkRate;
	};

	enum ConnectionState { Disconnected, Connecting, Idle, Working };
	enum SegmentState { None, Loading, HasData, MissingSegment, DownloadFailed };

	typedef std::function< void(const int32_t nzbID, const int32_t parts, const int32_t total) > MountStatusFunction;
	typedef std::function< void(const int32_t nzbID, const boost::filesystem::path& path) > NZBFileOpenFunction;
	typedef std::function< void(const int32_t nzbID) > NZBFileCloseFunction;
	typedef std::function< void(const int32_t nzbID, const int32_t fileID, const int32_t segments, const uint64_t size) > FileAddedFunction;
	typedef std::function< void(const int32_t fileID, const boost::filesystem::path& name, const uint64_t size) > FileInfoFunction;
	typedef std::function< void(const int32_t fileID, const int32_t segment, const SegmentState state) > FileSegmentStateChangedFunction;
	typedef std::function< void(const int32_t fileID) > FileRemovedFunction;
	typedef std::function< void(const ConnectionState state, const int32_t server, const int32_t thread) > ConnectionStateChangedFunction;
	typedef std::function< void(const LogLevel lvl, const std::string& msg)  > EventLogFunction;
	typedef std::function< void()> NZBDriveSharewareTimeout;
	typedef std::function< void(const int64_t time, const int32_t server, const uint64_t bytesTX, uint64_t bytesRX, 
		uint32_t missingSegmentCount, uint32_t connectionTimeoutCount, uint32_t errorCountg)> ConnectionInfoFunction;

	NZBDokanDrive();
	~NZBDokanDrive();

	Result Start();
	Result Stop();

	int32_t AddServer(const UsenetServer& server);
	void RemoveServer(const int32_t id);

	void OnTrialTimeout();

	int32_t Mount(const wchar_t* nzbpath, MountStatusFunction callback);
	int32_t Unmount(const wchar_t* nzbpath);

	std::wstring GetDriveLetter();
	void SetDriveLetter(const std::wstring&);
	bool GetPreCheckSegments();
	void SetPreCheckSegments(const bool);
	void SetNetworkThrottling(const NetworkThrottling&);
	uint_fast64_t GetRXBytes();
	LogLevel GetLogLevel();
	void SetLogLevel(LogLevel level);

	void SetMountStatusHandler(MountStatusFunction handler);
	void SetNZBFileOpenHandler(NZBFileOpenFunction handler);
	void SetNZBFileCloseHandler(NZBFileCloseFunction handler);
	void SetFileAddedHandler(FileAddedFunction handler);
	void SetFileInfoHandler(FileInfoFunction handler);
	void SetFileSegmentStateChangedHandler(FileSegmentStateChangedFunction handler);
	void SetFileRemovedHandler(FileRemovedFunction handler);
	void SetConnectionStateChangedHandler(ConnectionStateChangedFunction handler);
	void SetEventLogHandler(EventLogFunction handler);
	void SetConnectionInfoHandler(ConnectionInfoFunction handler);

	void SetCachePath(const boost::filesystem::path& cache_path);
	boost::filesystem::path GetCachePath() const;

};
