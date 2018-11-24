// NZBDokanDrive.cpp : Defines the entry point for the console application.
//
#undef _VARIADIC_MAX 
#define _VARIADIC_MAX  10

//#define WIN32_LEAN_AND_MEAN   /* Prevent inclusion of winsock.h in windows.h */

#include "NZBDokanDrive.hpp"
#include "IDirectory.hpp"
#include "Logger.hpp"
//#include "ntstatus.h"
//#include "windows.h"
#include "dokan.h"
#include <aclapi.h>
#include "fileinfo.h"
#include "NZBDrive.hpp"
#include "text_tool.hpp"
#include <memory>
#include <unordered_map>
#include <mutex>
#include <future>
#include <boost/algorithm/string.hpp>
#include <filesystem>
#define LODWORD(l) ((DWORD)((DWORDLONG)(l)))
#define HIDWORD(l) ((DWORD)(((DWORDLONG)(l)>>32)&0xFFFFFFFF))
#define VOL_SERIAL 0x19831116

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

struct DokanContext
{
	ULONG64 Handle;
	std::shared_ptr<ByteFountain::IFile> OpenFile;
	std::shared_ptr<ByteFountain::IDirectory> OpenDirectory;

	DokanContext() :Handle(0), OpenFile(nullptr), OpenDirectory(nullptr) {}
};

class INZBDokanDriveContext
{
public:
	virtual ~INZBDokanDriveContext(){}

	virtual ByteFountain::Logger& Logger() = 0;

	virtual std::shared_ptr<ByteFountain::IFile> GetFile(const ULONG64 handle) = 0;
	virtual std::shared_ptr<ByteFountain::IDirectory> GetDirectory(const ULONG64 handle) = 0;

	virtual ULONG64 MakeContext(std::shared_ptr<ByteFountain::IFile> file) = 0;
	virtual ULONG64 MakeContext(std::shared_ptr<ByteFountain::IDirectory> dir) = 0;
	virtual void RemoveContext(ULONG64 handle)=0;

	virtual ULONG CopySecurityDescriptorTo(ULONG lengthSD, PSECURITY_DESCRIPTOR pSecDesc)=0;

	virtual std::shared_ptr<ByteFountain::IFile> GetFile(const std::filesystem::path& path)=0;
	virtual std::shared_ptr<ByteFountain::IDirectory> GetDirectory(const std::filesystem::path& path)=0;
	virtual std::shared_ptr<ByteFountain::IDirectory> GetRootDirectory()=0;
	virtual unsigned long long GetTotalNumberOfBytes() const=0;


//	virtual void SignalMounted()=0;
};

namespace
{
	INZBDokanDriveContext* g_dokanContext;
	BOOL g_UseStdErr;
	BOOL g_DebugMode;

	void ReportException(INZBDokanDriveContext& context, const char* function)
	{
		try { throw; }
		catch (std::exception& e)
		{
			context.Logger() << ByteFountain::Logger::Error << "Exception in \"" << function << "\": " << e.what() << ByteFountain::Logger::End;
		}
		catch (...)
		{
			context.Logger() << ByteFountain::Logger::Error << "Exception in \"" << function << "\"" << ByteFountain::Logger::End;
		}
	}

}

static void DbgPrint(LPCWSTR format, ...)
{
	if (g_DebugMode) {
		WCHAR buffer[512];
		va_list argp;
		va_start(argp, format);
		vswprintf_s(buffer, sizeof(buffer)/sizeof(WCHAR), format, argp);
		va_end(argp);
		if (g_UseStdErr) {
			fwprintf(stderr, buffer);
		} else {
			OutputDebugStringW(buffer);
		}
	}
}

static void
PrintUserName(PDOKAN_FILE_INFO	DokanFileInfo)
{
#ifdef DEBUG
	HANDLE	handle;
	UCHAR buffer[5000];
	DWORD returnLength;
	WCHAR accountName[256];
	WCHAR domainName[256];
	DWORD accountLength = sizeof(accountName) / sizeof(WCHAR);
	DWORD domainLength = sizeof(domainName) / sizeof(WCHAR);
	PTOKEN_USER tokenUser;
	SID_NAME_USE snu;

	handle = DokanOpenRequestorToken(DokanFileInfo);
	if (handle == INVALID_HANDLE_VALUE) {
		DbgPrint(L"  DokanOpenRequestorToken failed\n");
		return;
	}

	if (!GetTokenInformation(handle, TokenUser, buffer, sizeof(buffer), &returnLength)) {
		DbgPrint(L"  GetTokenInformaiton failed: %d\n", GetLastError());
		CloseHandle(handle);
		return;
	}

	CloseHandle(handle);

	tokenUser = (PTOKEN_USER)buffer;
	if (!LookupAccountSid(NULL, tokenUser->User.Sid, accountName,
			&accountLength, domainName, &domainLength, &snu)) {
		DbgPrint(L"  LookupAccountSid failed: %d\n", GetLastError());
		return;
	}

	DbgPrint(L"  AccountName: %s, DomainName: %s\n", accountName, domainName);
#endif
}

#define NZBDokanDriveCheckFlag(val, flag) if (val&flag) { DbgPrint(L"\t" L#flag L"\n"); }



static NTSTATUS __stdcall
NZBDokanDriveCreateFile(LPCWSTR FileName,
	PDOKAN_IO_SECURITY_CONTEXT SecurityContext,
	ACCESS_MASK DesiredAccess,
	ULONG FileAttributes,
	ULONG ShareAccess,
	ULONG CreateDisposition,
	ULONG CreateOptions,
	PDOKAN_FILE_INFO DokanFileInfo)/*
(
	LPCWSTR					FileName,
	DWORD					AccessMode,
	DWORD					ShareMode,
	DWORD					CreationDisposition,
	DWORD					FlagsAndAttributes,
	PDOKAN_FILE_INFO		DokanFileInfo)*/
{
	DbgPrint(L"NZBDokanDriveCreateFile : %s\n", FileName);
//	g_dokanContext->Logger() << ByteFountain::Logger::Debug << "NZBDokanDriveCreateFile " << ByteFountain::Logger::End;

	try
	{

		if (DokanFileInfo->Context)
		{
			g_dokanContext->RemoveContext(DokanFileInfo->Context);
			DokanFileInfo->Context = 0;
		}

		PrintUserName(DokanFileInfo);

		if (CreateOptions == CREATE_NEW)
			DbgPrint(L"\tCREATE_NEW\n");
		if (CreateOptions == OPEN_ALWAYS)
			DbgPrint(L"\tOPEN_ALWAYS\n");
		if (CreateOptions == CREATE_ALWAYS)
			DbgPrint(L"\tCREATE_ALWAYS\n");
		if (CreateOptions == OPEN_EXISTING)
			DbgPrint(L"\tOPEN_EXISTING\n");
		if (CreateOptions == TRUNCATE_EXISTING)
			DbgPrint(L"\tTRUNCATE_EXISTING\n");

		DbgPrint(L"\tShareMode = 0x%x\n", ShareAccess);

		NZBDokanDriveCheckFlag(ShareAccess, FILE_SHARE_READ);
		NZBDokanDriveCheckFlag(ShareAccess, FILE_SHARE_WRITE);
		NZBDokanDriveCheckFlag(ShareAccess, FILE_SHARE_DELETE);

		DbgPrint(L"\tAccessMode = 0x%x\n", DesiredAccess);

		NZBDokanDriveCheckFlag(DesiredAccess, GENERIC_READ);
		NZBDokanDriveCheckFlag(DesiredAccess, GENERIC_WRITE);
		NZBDokanDriveCheckFlag(DesiredAccess, GENERIC_EXECUTE);

		NZBDokanDriveCheckFlag(DesiredAccess, DELETE);
		NZBDokanDriveCheckFlag(DesiredAccess, FILE_READ_DATA);
		NZBDokanDriveCheckFlag(DesiredAccess, FILE_READ_ATTRIBUTES);
		NZBDokanDriveCheckFlag(DesiredAccess, FILE_READ_EA);
		NZBDokanDriveCheckFlag(DesiredAccess, READ_CONTROL);
		NZBDokanDriveCheckFlag(DesiredAccess, FILE_WRITE_DATA);
		NZBDokanDriveCheckFlag(DesiredAccess, FILE_WRITE_ATTRIBUTES);
		NZBDokanDriveCheckFlag(DesiredAccess, FILE_WRITE_EA);
		NZBDokanDriveCheckFlag(DesiredAccess, FILE_APPEND_DATA);
		NZBDokanDriveCheckFlag(DesiredAccess, WRITE_DAC);
		NZBDokanDriveCheckFlag(DesiredAccess, WRITE_OWNER);
		NZBDokanDriveCheckFlag(DesiredAccess, SYNCHRONIZE);
		NZBDokanDriveCheckFlag(DesiredAccess, FILE_EXECUTE);
		NZBDokanDriveCheckFlag(DesiredAccess, STANDARD_RIGHTS_READ);
		NZBDokanDriveCheckFlag(DesiredAccess, STANDARD_RIGHTS_WRITE);
		NZBDokanDriveCheckFlag(DesiredAccess, STANDARD_RIGHTS_EXECUTE);

		NZBDokanDriveCheckFlag(FileAttributes, FILE_ATTRIBUTE_ARCHIVE);
		NZBDokanDriveCheckFlag(FileAttributes, FILE_ATTRIBUTE_ENCRYPTED);
		NZBDokanDriveCheckFlag(FileAttributes, FILE_ATTRIBUTE_HIDDEN);
		NZBDokanDriveCheckFlag(FileAttributes, FILE_ATTRIBUTE_NORMAL);
		NZBDokanDriveCheckFlag(FileAttributes, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
		NZBDokanDriveCheckFlag(FileAttributes, FILE_ATTRIBUTE_OFFLINE);
		NZBDokanDriveCheckFlag(FileAttributes, FILE_ATTRIBUTE_READONLY);
		NZBDokanDriveCheckFlag(FileAttributes, FILE_ATTRIBUTE_SYSTEM);
		NZBDokanDriveCheckFlag(FileAttributes, FILE_ATTRIBUTE_TEMPORARY);
		NZBDokanDriveCheckFlag(FileAttributes, FILE_FLAG_WRITE_THROUGH);
		NZBDokanDriveCheckFlag(FileAttributes, FILE_FLAG_OVERLAPPED);
		NZBDokanDriveCheckFlag(FileAttributes, FILE_FLAG_NO_BUFFERING);
		NZBDokanDriveCheckFlag(FileAttributes, FILE_FLAG_RANDOM_ACCESS);
		NZBDokanDriveCheckFlag(FileAttributes, FILE_FLAG_SEQUENTIAL_SCAN);
		NZBDokanDriveCheckFlag(FileAttributes, FILE_FLAG_DELETE_ON_CLOSE);
		NZBDokanDriveCheckFlag(FileAttributes, FILE_FLAG_BACKUP_SEMANTICS);
		NZBDokanDriveCheckFlag(FileAttributes, FILE_FLAG_POSIX_SEMANTICS);
		NZBDokanDriveCheckFlag(FileAttributes, FILE_FLAG_OPEN_REPARSE_POINT);
		NZBDokanDriveCheckFlag(FileAttributes, FILE_FLAG_OPEN_NO_RECALL);
		NZBDokanDriveCheckFlag(FileAttributes, SECURITY_ANONYMOUS);
		NZBDokanDriveCheckFlag(FileAttributes, SECURITY_IDENTIFICATION);
		NZBDokanDriveCheckFlag(FileAttributes, SECURITY_IMPERSONATION);
		NZBDokanDriveCheckFlag(FileAttributes, SECURITY_DELEGATION);
		NZBDokanDriveCheckFlag(FileAttributes, SECURITY_CONTEXT_TRACKING);
		NZBDokanDriveCheckFlag(FileAttributes, SECURITY_EFFECTIVE_ONLY);
		NZBDokanDriveCheckFlag(FileAttributes, SECURITY_SQOS_PRESENT);

		std::filesystem::path path(FileName);

		std::shared_ptr<ByteFountain::IDirectory> subdir;
		std::shared_ptr<ByteFountain::IFile> file;

		if (subdir = g_dokanContext->GetDirectory(path))
		{
			DbgPrint(L"\tDirectory opened: %s\n", FileName);
			DokanFileInfo->IsDirectory = true;
			DokanFileInfo->Context = g_dokanContext->MakeContext(subdir);
			return ERROR_SUCCESS;
		}
		else if (file = g_dokanContext->GetFile(path))
		{
			DbgPrint(L"\tFile opened: %s\n", FileName);
			DokanFileInfo->IsDirectory = false;
			DokanFileInfo->Context = g_dokanContext->MakeContext(file);
			return ERROR_SUCCESS;
		}
		return -ERROR_FILE_NOT_FOUND;
	}
	catch (...)
	{
		ReportException(*g_dokanContext, "NZBDokanDriveCreateFile");
		return -ERROR_INVALID_FUNCTION;
	}
}
/*
static int __stdcall
NZBDokanDriveOpenDirectory(
	LPCWSTR					FileName,
	PDOKAN_FILE_INFO		DokanFileInfo)
{
	DbgPrint(L"NZBDokanDriveOpenDirectory : %s\n", FileName);
//	g_dokanContext->Logger() << ByteFountain::Logger::Debug << "NZBDokanDriveOpenDirectory " << ByteFountain::Logger::End;

	try
	{
		if (DokanFileInfo->Context) 
		{
			g_dokanContext->RemoveContext(DokanFileInfo->Context);
			DokanFileInfo->Context = 0;
		}

		std::filesystem::path path(FileName);
		std::shared_ptr<ByteFountain::IDirectory> subdir=g_dokanContext->GetDirectory(path);
		if (subdir)
		{
			DbgPrint(L"\tDirectory opened: %s\n", FileName);
			DokanFileInfo->IsDirectory=true;
			DokanFileInfo->Context=g_dokanContext->MakeContext(subdir);
			return ERROR_SUCCESS;
		}
		return -ERROR_PATH_NOT_FOUND;
	}
	catch (...)
	{
		ReportException(*g_dokanContext, "NZBDokanDriveOpenDirectory");
		return -ERROR_INVALID_FUNCTION;
	}

}
*/

static void __stdcall
NZBDokanDriveCloseFile(
	LPCWSTR					FileName,
	PDOKAN_FILE_INFO		DokanFileInfo)
{
	DbgPrint(L"NZBDokanDriveCloseFile: %s\n", FileName);
//	g_dokanContext->Logger() << ByteFountain::Logger::Debug << "NZBDokanDriveCloseFile " << ByteFountain::Logger::End;

	try
	{
		if (DokanFileInfo->Context)
		{
			DbgPrint(L"\tClosing file: %s\n", FileName);
			g_dokanContext->RemoveContext(DokanFileInfo->Context);
			DokanFileInfo->Context = 0;
		}
		else
		{
			DbgPrint(L"\tClosing file (invalid handle): %s\n", FileName);
		}
	}
	catch (...)
	{
		ReportException(*g_dokanContext, "NZBDokanDriveCloseFile");
	}

}


static void __stdcall
NZBDokanDriveCleanup(
	LPCWSTR					FileName,
	PDOKAN_FILE_INFO		DokanFileInfo)
{

	DbgPrint(L"NZBDokanDriveCleanup: %s\n", FileName);
//	g_dokanContext->Logger() << ByteFountain::Logger::Debug << "NZBDokanDriveCleanup " << ByteFountain::Logger::End;

	try
	{
		if (DokanFileInfo->Context) 
		{
			DbgPrint(L"\tDrive cleanup: %s\n", FileName);
			g_dokanContext->RemoveContext(DokanFileInfo->Context);
			DokanFileInfo->Context = 0;
		}
		else
		{
			DbgPrint(L"\tDrive cleanup (invalid handle): %s\n", FileName);
		}
	}
	catch (...)
	{
		ReportException(*g_dokanContext, "NZBDokanDriveCleanup");
	}
}


static NTSTATUS __stdcall
NZBDokanDriveReadFile(LPCWSTR FileName,
	LPVOID Buffer,
	DWORD BufferLength,
	LPDWORD ReadLength,
	LONGLONG Offset,
	PDOKAN_FILE_INFO DokanFileInfo)
	/*(
	LPCWSTR				FileName,
	LPVOID				Buffer,
	DWORD				BufferLength,
	LPDWORD				ReadLength,
	LONGLONG			Offset,
	PDOKAN_FILE_INFO	DokanFileInfo)*/
{
	DbgPrint(L"NZBDokanDriveReadFile: %s\n", FileName);
//	g_dokanContext->Logger() << ByteFountain::Logger::Debug << "NZBDokanDriveReadFile " << ByteFountain::Logger::End;

	try
	{
		std::shared_ptr<ByteFountain::IFile> file;

		if (DokanFileInfo->Context)
		{
			file = g_dokanContext->GetFile(DokanFileInfo->Context);
		}

		if (!file)
		{
			std::filesystem::path path(FileName);
			file = g_dokanContext->GetFile(path);
			if (file)
			{
				DokanFileInfo->IsDirectory = false;
				DokanFileInfo->Context = g_dokanContext->MakeContext(file);
				return ERROR_SUCCESS;
			}
			else
			{
				DbgPrint(L"\tCould not find file: %s\n", FileName);
				return -ERROR_FILE_NOT_FOUND;
			}

			DbgPrint(L"\tReading from: %s\n", FileName);
		}

		if (file)
		{
			size_t readsize;
			// TODO: Check typecast of Offset...
			if (file->GetFileData((char*)Buffer, (unsigned long long)Offset, BufferLength, readsize))
			{
				DbgPrint(L"\tREAD ERROR: %s\n", FileName);
			}
			*ReadLength = (DWORD)readsize;
			return ERROR_SUCCESS;
		}
		return -ERROR_INVALID_FUNCTION;
	}
	catch (...)
	{
		ReportException(*g_dokanContext, "NZBDokanDriveReadFile");
		return -ERROR_INVALID_FUNCTION;
	}
}

static NTSTATUS __stdcall
NZBDokanDriveGetFileInformation(
	LPCWSTR							FileName,
	LPBY_HANDLE_FILE_INFORMATION	HandleFileInformation,
	PDOKAN_FILE_INFO				DokanFileInfo)
{
	DbgPrint(L"NZBDokanDriveFileInformation: %s\n", FileName);
//	g_dokanContext->Logger() << ByteFountain::Logger::Debug << "NZBDokanDriveGetFileInformation " << ByteFountain::Logger::End;

	try
	{
		std::shared_ptr<ByteFountain::IFile> file;
		std::shared_ptr<ByteFountain::IDirectory> directory;

		if (DokanFileInfo->Context)
		{
			file = g_dokanContext->GetFile(DokanFileInfo->Context);
			if (!file) directory = g_dokanContext->GetDirectory(DokanFileInfo->Context);
		}
		else
		{
			std::string filename=ByteFountain::text_tool::unicode_to_utf8(FileName);
			std::filesystem::path path(filename);
			directory=g_dokanContext->GetDirectory(path);
			file=g_dokanContext->GetFile(path);
		}

		if (file)
		{
			HandleFileInformation->dwFileAttributes = FILE_ATTRIBUTE_READONLY;
			HandleFileInformation->dwVolumeSerialNumber = VOL_SERIAL;
			HandleFileInformation->ftCreationTime.dwLowDateTime = 0;
			HandleFileInformation->ftCreationTime.dwHighDateTime = 0;
			HandleFileInformation->ftLastAccessTime.dwLowDateTime = 0;
			HandleFileInformation->ftLastAccessTime.dwHighDateTime = 0;
			HandleFileInformation->ftLastWriteTime.dwLowDateTime = 0;
			HandleFileInformation->ftLastWriteTime.dwHighDateTime = 0;
			HandleFileInformation->nFileIndexHigh = 0; //TODO??
			HandleFileInformation->nFileIndexLow = 0; //TODO??

			unsigned long long size=file->GetFileSize();

			HandleFileInformation->nFileSizeLow = LODWORD(size);
			HandleFileInformation->nFileSizeHigh = HIDWORD(size);
			return ERROR_SUCCESS;
		}
		else if (directory)
		{
			HandleFileInformation->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
			HandleFileInformation->dwVolumeSerialNumber = VOL_SERIAL;
			HandleFileInformation->ftCreationTime.dwLowDateTime = 0;
			HandleFileInformation->ftCreationTime.dwHighDateTime = 0;
			HandleFileInformation->ftLastAccessTime.dwLowDateTime = 0;
			HandleFileInformation->ftLastAccessTime.dwHighDateTime = 0;
			HandleFileInformation->ftLastWriteTime.dwLowDateTime = 0;
			HandleFileInformation->ftLastWriteTime.dwHighDateTime = 0;
			HandleFileInformation->nFileIndexHigh = 0; //TODO??
			HandleFileInformation->nFileIndexLow = 0; //TODO??
			HandleFileInformation->nFileSizeLow = LODWORD(0);
			HandleFileInformation->nFileSizeHigh = HIDWORD(0);
			return ERROR_SUCCESS;
		}

		return -ERROR_FILE_NOT_FOUND;
	}
	catch (...)
	{
		ReportException(*g_dokanContext, "NZBDokanDriveGetFileInformation");
		return -ERROR_INVALID_FUNCTION;
	}
}


static NTSTATUS __stdcall
NZBDokanDriveFindFiles(
	LPCWSTR				FileName,
	PFillFindData		FillFindData, // function pointer
	PDOKAN_FILE_INFO	DokanFileInfo)
{
	DbgPrint(L"NZBDokanDriveFindFiles: %s\n", FileName);
//	g_dokanContext->Logger() << ByteFountain::Logger::Debug << "NZBDokanDriveFindFiles " << ByteFountain::Logger::End;

	try
	{
		WIN32_FIND_DATAW	findData;
		PWCHAR				yenStar = L"\\*";
		int count = 0;

		std::filesystem::path path(FileName);
		std::shared_ptr<ByteFountain::IDirectory> subdir=g_dokanContext->GetDirectory(path);
	
		if (!subdir) return ERROR_SUCCESS;

		std::vector< std::pair<std::filesystem::path,ByteFountain::ContentType> > content=subdir->GetContent();

		if (subdir!=g_dokanContext->GetRootDirectory())
		{
			findData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
			findData.ftCreationTime.dwLowDateTime = 0;
			findData.ftCreationTime.dwHighDateTime = 0;
			findData.ftLastAccessTime.dwLowDateTime = 0;
			findData.ftLastAccessTime.dwHighDateTime = 0;
			findData.ftLastWriteTime.dwLowDateTime = 0;
			findData.ftLastWriteTime.dwHighDateTime = 0;
			findData.nFileSizeLow = LODWORD(0);
			findData.nFileSizeHigh = HIDWORD(0);
			findData.dwReserved0=0;
			findData.dwReserved1=0;
			wcsncpy_s(findData.cFileName, MAX_PATH, L".", 1);
			wcsncpy_s(findData.cAlternateFileName, 14, L".",_TRUNCATE);
			FillFindData(&findData, DokanFileInfo);
			wcsncpy_s(findData.cFileName, MAX_PATH, L"..", 2);
			wcsncpy_s(findData.cAlternateFileName, 14, L"..",_TRUNCATE);
			FillFindData(&findData, DokanFileInfo);
		}

		for(const auto& e : content)
		{
			const std::filesystem::path& mypath(e.first);
			ByteFountain::ContentType type(e.second);

			findData.ftCreationTime.dwLowDateTime = 0;
			findData.ftCreationTime.dwHighDateTime = 0;
			findData.ftLastAccessTime.dwLowDateTime = 0;
			findData.ftLastAccessTime.dwHighDateTime = 0;
			findData.ftLastWriteTime.dwLowDateTime = 0;
			findData.ftLastWriteTime.dwHighDateTime = 0;

			findData.dwReserved0=0;
			findData.dwReserved1=0;

			unsigned long long size=0;

			if (type==ByteFountain::ContentType::File)
			{
				findData.dwFileAttributes = FILE_ATTRIBUTE_READONLY;
				std::shared_ptr<ByteFountain::IFile> file=g_dokanContext->GetFile(path/mypath);
				if (file.get()!=0) size=file->GetFileSize();
			}
			else
			{
				findData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
			}

			findData.nFileSizeLow = LODWORD(size);
			findData.nFileSizeHigh = HIDWORD(size);

			std::wstring wfilename=mypath.c_str();
			wcsncpy_s(findData.cFileName, MAX_PATH, wfilename.c_str(), wfilename.length());
			wcsncpy_s(findData.cAlternateFileName, 14, wfilename.c_str(),_TRUNCATE);
			
			FillFindData(&findData, DokanFileInfo);

		}
		return ERROR_SUCCESS;
	}
	catch (...)
	{
		ReportException(*g_dokanContext, "NZBDokanDriveFindFiles");
		return -ERROR_INVALID_FUNCTION;
	}
}

static NTSTATUS __stdcall
NZBDokanDriveGetVolumeInformation(
	LPWSTR		VolumeNameBuffer,
	DWORD		VolumeNameSize,
	LPDWORD		VolumeSerialNumber,
	LPDWORD		MaximumComponentLength,
	LPDWORD		FileSystemFlags,
	LPWSTR		FileSystemNameBuffer,
	DWORD		FileSystemNameSize,
	PDOKAN_FILE_INFO	DokanFileInfo)
{
	DbgPrint(L"NZBDokanDriveGetVolumeInformation\n");
//	g_dokanContext->Logger() << ByteFountain::Logger::Debug << "NZBDokanDriveGetVolumeInformation " << ByteFountain::Logger::End;

	try
	{
		wcscpy_s(VolumeNameBuffer, VolumeNameSize / sizeof(WCHAR), L"NZBDrive");
		*VolumeSerialNumber = VOL_SERIAL;
		*MaximumComponentLength = 256;
		*FileSystemFlags = FILE_CASE_SENSITIVE_SEARCH | 
							FILE_CASE_PRESERVED_NAMES | 
							FILE_SUPPORTS_REMOTE_STORAGE |
							FILE_UNICODE_ON_DISK |
							FILE_PERSISTENT_ACLS | FILE_READ_ONLY_VOLUME;

		wcscpy_s(FileSystemNameBuffer, FileSystemNameSize / sizeof(WCHAR), L"NZBDrive");

		return ERROR_SUCCESS;
	}
	catch (...)
	{
		ReportException(*g_dokanContext, "NZBDokanDriveGetVolumeInformation");
		return -ERROR_INVALID_FUNCTION;
	}
}


static NTSTATUS __stdcall
NZBDokanDriveUnmount(
	PDOKAN_FILE_INFO	DokanFileInfo)
{
	DbgPrint(L"NZBDokanDriveUnmount\n");
//	g_dokanContext->Logger() << ByteFountain::Logger::Debug << "NZBDokanDriveUnmount " << ByteFountain::Logger::End;

	try
	{
		if (DokanFileInfo->Context) 
		{
			g_dokanContext->RemoveContext(DokanFileInfo->Context);
			DokanFileInfo->Context = 0;
			DbgPrint(L"\tDrive closed: ?????????\n");
		}

		return ERROR_SUCCESS;
	}
	catch (...)
	{
		ReportException(*g_dokanContext, "NZBDokanDriveUnmount");
		return -ERROR_INVALID_FUNCTION;
	}
}


static NTSTATUS __stdcall
NZBGetFileSecurity(
LPCWSTR FileName, // FileName
PSECURITY_INFORMATION pSecInfo, // A pointer to SECURITY_INFORMATION value being requested
PSECURITY_DESCRIPTOR pSecDesc, // A pointer to SECURITY_DESCRIPTOR buffer to be filled
ULONG SecDesc, // length of Security descriptor buffer
PULONG pSecDescLen, // LengthNeeded
PDOKAN_FILE_INFO DokanFileInfo
)
{
	DbgPrint(L"NZBGetFileSecurity\n");
//	g_dokanContext->Logger() << ByteFountain::Logger::Info << "NZBGetFileSecurity " << oss.str() << ByteFountain::Logger::End;

	ULONG len = g_dokanContext->CopySecurityDescriptorTo(SecDesc, pSecDesc);

	if (pSecDescLen!=NULL) *pSecDescLen = len;

	return ERROR_SUCCESS;
}

static NTSTATUS __stdcall
NZBGetDiskFreeSpace(
	PULONGLONG pFB, // FreeBytesAvailable
	PULONGLONG pTN, // TotalNumberOfBytes
	PULONGLONG pTF, // TotalNumberOfFreeBytes
	PDOKAN_FILE_INFO DokanFileInfo
)
{ 
	DbgPrint(L"NZBGetDiskFreeSpace\n");
//	g_dokanContext->Logger() << ByteFountain::Logger::Debug << "NZBGetDiskFreeSpace " << ByteFountain::Logger::End;
	try
	{
		if (pFB != 0) pFB = 0;
		if (pTN!=0)
		{
				*pTN = g_dokanContext->GetTotalNumberOfBytes();
		}
		if (pTF!=0) pTF=0;

		return ERROR_SUCCESS;
	}
	catch (...)
	{
		ReportException(*g_dokanContext, "NZBGetDiskFreeSpace");
		return -ERROR_INVALID_FUNCTION;
	}
};


namespace // CREATE SECURITY DESCRIPTOR:
{

	HANDLE GetLogonToken()
	{
		HANDLE hToken = NULL;
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
		{
			return NULL;
		}
		return hToken;
	}

	static auto localFreeDeletor = [](VOID* psid){ LocalFree(psid); };

	std::unique_ptr<SID, decltype(localFreeDeletor)> GetUserTokenSID(HANDLE hToken)
	{

		std::unique_ptr<SID, decltype(localFreeDeletor)> res((SID*)0, localFreeDeletor);

		std::unique_ptr<TOKEN_USER, decltype(localFreeDeletor)> ptu((TOKEN_USER*)0, localFreeDeletor);

		DWORD dwLength = 0;

		// Get required buffer size and allocate the TOKEN_GROUPS buffer.

		if (!GetTokenInformation(
			hToken,         // handle to the access token
			TokenUser,    // get information about the token's groups 
			NULL,   // pointer to TOKEN_GROUPS buffer
			0,              // size of buffer
			&dwLength       // receives required buffer size
			))
		{
			if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return res;

			ptu.reset((TOKEN_USER*)LocalAlloc(LMEM_FIXED, dwLength));
		}

		// Get the token group information from the access token.

		if (!GetTokenInformation(
			hToken,         // handle to the access token
			TokenUser,    // get information about the token's groups 
			(LPVOID)ptu.get(),   // pointer to TOKEN_GROUPS buffer
			dwLength,       // size of buffer
			&dwLength       // receives required buffer size
			))
		{
			return res;
		}

		dwLength = GetLengthSid(ptu->User.Sid);

		res.reset((SID*)LocalAlloc(LMEM_FIXED, dwLength));

		if (!CopySid(dwLength, res.get(), ptu->User.Sid))
		{
			res.reset();
		}

		return res;
	}

	std::unique_ptr<SID, decltype(localFreeDeletor)> CreateWellKnownSid(const WELL_KNOWN_SID_TYPE sid)
	{
		DWORD SidSize = SECURITY_MAX_SID_SIZE;
		std::unique_ptr<SID, decltype(localFreeDeletor)> res((SID*)LocalAlloc(LMEM_FIXED, SidSize), localFreeDeletor);
		if (!CreateWellKnownSid(sid, NULL, res.get(), &SidSize)) res.reset();
		return res;
	}

	TRUSTEE Trustee(const PSID psid)
	{
		TRUSTEE trustee;
		BuildTrusteeWithSid(&trustee, psid);
		return trustee;
	}

}



class NZBDokanDrive_IMPL :public INZBDokanDriveContext
{
	NZBDokanDrive::MountStatusFunction m_mountStatusFunction;
	NZBDokanDrive::EventLogFunction m_eventLogFunction;

	ByteFountain::Logger m_logger;

	std::shared_ptr<ByteFountain::NZBDrive> m_NZBDrive;

	enum Status { Stopped, StartingDrive, StartingDokan, Started, StoppingDokan, StoppingDrive };

	Status m_status;
	std::mutex m_statusMutex;
//	std::condition_variable m_statusCv;

	DOKAN_OPERATIONS m_dokanOperations;
	DOKAN_OPTIONS m_dokanOptions;
	std::thread m_thread;
	int m_dokan_status;

	//settings:
	std::wstring m_drive_letter;
	bool m_pre_check_segments;
	ByteFountain::NetworkThrottling m_throttling;;

	WCHAR m_mount_point[MAX_PATH];

	// File access security descriptor:
	ULONG m_securityDescriptorLength;
	std::unique_ptr<SECURITY_DESCRIPTOR, decltype(localFreeDeletor)> m_securityDescriptor;

	// IDokanContextStore:
	std::unordered_map<ULONG64, DokanContext> m_dokanContexts;
	std::mutex m_dokanContextsMutex;
	std::atomic<ULONG64> m_contextCounter;

	NZBDokanDrive::Result ConvertDokanStatus(const int dokan_status)
	{
		switch (dokan_status) 
		{
			case DOKAN_SUCCESS:
				return NZBDokanDrive::Success;
			case DOKAN_ERROR:
				m_logger<<ByteFountain::Logger::PopupError<<"Error occured when mounting drive"<<ByteFountain::Logger::End;
				return NZBDokanDrive::MountDokanError;
			case DOKAN_DRIVE_LETTER_ERROR:
				m_logger<<ByteFountain::Logger::PopupError<<"Error occured when mounting drive, bad drive letter"<<ByteFountain::Logger::End;
				return NZBDokanDrive::MountDriveLetterError;
			case DOKAN_DRIVER_INSTALL_ERROR:
				m_logger<<ByteFountain::Logger::PopupError<<"Error occured when mounting drive, cannot install driver"<<ByteFountain::Logger::End;
				return NZBDokanDrive::MountDriverInstallError;
			case DOKAN_START_ERROR:
				m_logger<<ByteFountain::Logger::PopupError<<"Error occured when mounting drive, driver is malfunctioning"<<ByteFountain::Logger::End;
				return NZBDokanDrive::MountStartError;
			case DOKAN_MOUNT_ERROR:
				m_logger<<ByteFountain::Logger::PopupError<<"Error occured when mounting drive, cannot assign driver letter"<<ByteFountain::Logger::End;
				return NZBDokanDrive::MountError;
			case DOKAN_MOUNT_POINT_ERROR:
				m_logger<<ByteFountain::Logger::PopupError<<"Error occured when mounting drive, mount point error"<<ByteFountain::Logger::End;
				return NZBDokanDrive::MountPointError;
			default:
				m_logger<<ByteFountain::Logger::PopupError<<"Error occured when mounting drive, unknow error "<<m_status<<ByteFountain::Logger::End;
				return NZBDokanDrive::MountDokanError;
		}
	}
	void InitializeSecurityDescriptor()
	{
		EXPLICIT_ACCESS grantRW;
		grantRW.grfAccessPermissions = GENERIC_READ | GENERIC_EXECUTE;
		grantRW.grfAccessMode = GRANT_ACCESS;
		grantRW.grfInheritance = NO_INHERITANCE;
		//	grantRW.Trustee = ;

		std::vector<EXPLICIT_ACCESS> accessRights;

		auto closeHandleDeleter = [](HANDLE h){ CloseHandle(h); };

		std::unique_ptr<std::remove_pointer<HANDLE>::type, decltype(closeHandleDeleter)> userToken(GetLogonToken(), closeHandleDeleter);

		if (!userToken)
		{
			m_logger << ByteFountain::Logger::Info << "Unable to get Logged on user token " << ByteFountain::Logger::End;
		}

		auto sidUser = GetUserTokenSID(userToken.get());

		if (!sidUser)
		{
			m_logger << ByteFountain::Logger::Error << "Unable to get User SID" << ByteFountain::Logger::End;
		}
		else
		{
			accessRights.push_back(grantRW);
			accessRights.back().Trustee = Trustee(sidUser.get());
		}

		auto sidSystem = CreateWellKnownSid(WinLocalSystemSid);

		if (!sidSystem)
		{
			m_logger << ByteFountain::Logger::Error << "Unable to get System SID" << ByteFountain::Logger::End;
		}
		else
		{
			accessRights.push_back(grantRW);
			accessRights.back().Trustee = Trustee(sidSystem.get());
		}

		auto sidAdmins = CreateWellKnownSid(WinBuiltinAdministratorsSid);

		if (!sidAdmins)
		{
			m_logger << ByteFountain::Logger::Error << "Unable to get Administrators SID" << ByteFountain::Logger::End;
		}
		else
		{
			accessRights.push_back(grantRW);
			accessRights.back().Trustee = Trustee(sidAdmins.get());
		}

		auto sidWorld = CreateWellKnownSid(WinBuiltinUsersSid);

		if (!sidWorld)
		{
			m_logger << ByteFountain::Logger::Error << "Unable to get Users SID" << ByteFountain::Logger::End;
		}
		else
		{
			accessRights.push_back(grantRW);
			accessRights.back().Trustee = Trustee(sidWorld.get());
		}

		ULONG lengthSD = 0;
		PSECURITY_DESCRIPTOR pSD = NULL;

		if (ERROR_SUCCESS != BuildSecurityDescriptor(sidUser ? &accessRights[0].Trustee : NULL, NULL, 
			(ULONG)accessRights.size(), &accessRights[0], 0, NULL, NULL, &lengthSD, &pSD))
		{
			m_logger << ByteFountain::Logger::Error << "BuildSecurityDescriptor failed" << ByteFountain::Logger::End;
		}

		m_securityDescriptorLength = lengthSD;
		m_securityDescriptor.reset((SECURITY_DESCRIPTOR*)pSD);

	}


public:
#pragma warning ( disable : 4351 )
	NZBDokanDrive_IMPL() :
		m_logger([this](const ByteFountain::LogLineLevel lvl, const std::string& msg)
			{
				SendEvent(m_eventLogFunction, adapt(lvl), msg);
			}
		),
		m_NZBDrive(new ByteFountain::NZBDrive(m_logger)),
		m_status(Stopped),
		m_dokanOperations(),
		m_dokanOptions(),
		m_thread(),
		m_dokan_status(DOKAN_ERROR),
		m_drive_letter(L"N"),
		m_pre_check_segments(false),
		m_mount_point(),
		m_securityDescriptorLength(0),
		m_securityDescriptor(0, localFreeDeletor),
		m_dokanContexts(),
		m_dokanContextsMutex(),
		m_contextCounter(0)
	{
		ZeroMemory((void*)&m_dokanOperations, sizeof(DOKAN_OPERATIONS));
		ZeroMemory((void*)&m_dokanOptions, sizeof(DOKAN_OPTIONS));

		m_dokanOptions.Version = DOKAN_VERSION;
		m_dokanOptions.ThreadCount = 0; // use default

		m_dokanOptions.MountPoint = m_mount_point;

		m_dokanOptions.ThreadCount = (USHORT)3;

		InitializeSecurityDescriptor();

#ifdef _DEBUG

		g_DebugMode = TRUE;
		g_UseStdErr = TRUE;

#else

		g_DebugMode = FALSE;
		g_UseStdErr = FALSE;

#endif

		if (g_DebugMode)
		{
			m_dokanOptions.Options |= DOKAN_OPTION_DEBUG;
		}
		if (g_UseStdErr)
		{
			m_dokanOptions.Options |= DOKAN_OPTION_STDERR;
		}

//		m_dokanOptions.Options |= DOKAN_OPTION_KEEP_ALIVE;

		m_dokanOperations.ZwCreateFile = NZBDokanDriveCreateFile;
//		m_dokanOperations.OpenDirectory = NZBDokanDriveOpenDirectory;
		m_dokanOperations.Cleanup = NZBDokanDriveCleanup;
		m_dokanOperations.CloseFile = NZBDokanDriveCloseFile;
		m_dokanOperations.ReadFile = NZBDokanDriveReadFile;
		m_dokanOperations.GetFileInformation = NZBDokanDriveGetFileInformation;
		m_dokanOperations.FindFiles = NZBDokanDriveFindFiles;
		m_dokanOperations.GetVolumeInformation = NZBDokanDriveGetVolumeInformation;
		m_dokanOperations.Unmounted = NZBDokanDriveUnmount;
		m_dokanOperations.GetFileSecurity = NZBGetFileSecurity;
		m_dokanOperations.GetDiskFreeSpace = NZBGetDiskFreeSpace;
	}

	ByteFountain::Logger& Logger()
	{
		return m_logger;
	}

#pragma warning ( default : 4351 )

	~NZBDokanDrive_IMPL()
	{
		m_eventLogFunction = nullptr;
		Stop();
		m_NZBDrive.reset();
	}

	void OnEventLog(const ByteFountain::LogLineLevel lvl, const std::string& message)
	{
		SendEvent(m_eventLogFunction, adapt(lvl), message);
	}

	NZBDokanDrive::Result Start()
	{
		std::unique_lock<std::mutex> lk(m_statusMutex);

		m_NZBDrive->SetNetworkThrottling(m_throttling);

		if (!m_NZBDrive->Start()) return NZBDokanDrive::DriveNotStarted;

		if (!_SetDriveLetter(m_drive_letter)) return NZBDokanDrive::DriveNotStarted;

		return NZBDokanDrive::Success;
	}

	NZBDokanDrive::Result Stop(Status reason= Stopped)
	{
		std::unique_lock<std::mutex> lk(m_statusMutex);

		if (m_status==Stopped) return NZBDokanDrive::Success;

		_SetDriveLetter(L"");

		m_status=StoppingDrive;
		m_NZBDrive->Stop();
		ClearDokanContext();
		m_status=reason;

//		m_eventLogFunction = nullptr;

		return ConvertDokanStatus(m_dokan_status);
	}

	std::wstring GetDriveLetter() { return m_drive_letter; }
	bool _SetDriveLetter(const std::wstring& val)
	{
		if (val.size() > 1) return false;

		if (m_thread.joinable() && val == m_drive_letter) return true; // Already running.

		if (m_thread.joinable())
		{
			m_status = StoppingDokan;
			DokanRemoveMountPoint(m_dokanOptions.MountPoint);
			m_thread.join(); // TODO: Investigate hang here... Seems that dokan.cpp line 238 hangs.
		}

		if (val.empty())
		{
			m_drive_letter = L"";
			return true;
		}

		std::wstring wmount_point = val + L":\\";

		if (IsDrive(wmount_point)) return false;

		m_drive_letter = val;
		wmount_point = wmount_point;
		wcscpy_s(m_mount_point, sizeof(m_mount_point) / sizeof(WCHAR), wmount_point.c_str());

		m_status = StartingDokan;
		g_dokanContext = this;

		m_dokan_status = DOKAN_SUCCESS;

		m_thread = std::thread([this]()
		{
			m_dokan_status = DokanMain(&m_dokanOptions, &m_dokanOperations);
			g_dokanContext = 0;
		});

		int count = 0;

		while (!IsDrive(wmount_point) && count++<100)
		{
			Sleep(100);
		}

		if (!IsDrive(wmount_point))
		{
			m_status = StoppingDokan;
			DokanRemoveMountPoint(m_dokanOptions.MountPoint);
			m_thread.join();
			m_status = Stopped;
			return false;
		}

		m_status = Started;
		m_dokan_status = DOKAN_SUCCESS;

		return true;
	}


	bool SetDriveLetter(const std::wstring& val)
	{ 
		std::unique_lock<std::mutex> lk(m_statusMutex);
		return _SetDriveLetter(val);
	}

	int32_t Mount(const wchar_t* nzbpath, NZBDokanDrive::MountStatusFunction callback)
	{
		std::unique_lock<std::mutex> lk(m_statusMutex);

		if (m_status!=Started)
		{
			m_logger<<ByteFountain::Logger::PopupError<<"Drive was not started"<<ByteFountain::Logger::End;
			return NZBDokanDrive::DriveNotStarted;
		}

		std::filesystem::path mynzbpath(nzbpath);
		std::filesystem::path mynzbmount(mynzbpath.filename());

		return m_NZBDrive->Mount(mynzbmount,mynzbpath.string(), 
			[callback](const int32_t nzbID, const int32_t parts, const int32_t total)
			{
				callback(nzbID,parts,total);
			});
	}
	int32_t Unmount(const wchar_t* nzbpath)
	{
		std::unique_lock<std::mutex> lk(m_statusMutex);
		if (m_status!=Started) return NZBDokanDrive::DriveNotStarted;

		std::filesystem::path mynzbpath(nzbpath);
		std::filesystem::path mynzbmount(mynzbpath.filename());

		int32_t res = m_NZBDrive->Unmount(mynzbmount);

		ClearDokanContext();
		return res;
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
		return m_NZBDrive->AddServer(srv);
	}

	void RemoveServer(const int32_t id)
	{
		m_NZBDrive->RemoveServer(id);
	}

	bool GetPreCheckSegments(){ return m_pre_check_segments; }
	void SetPreCheckSegments(const bool val){ m_pre_check_segments=val; }
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

		m_NZBDrive->SetNetworkThrottling(m_throttling);
	}
	uint_fast64_t GetRXBytes(){ return m_NZBDrive->RXBytes(); }
//	bool CheckLicenseKey(const std::string& val) { return ByteFountain::NZBDriveCheckLicenseKey(val); }


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
		m_NZBDrive->SetNZBFileOpenHandler(RelayEvent(handler));
	}
	void SetNZBFileCloseHandler(NZBDokanDrive::NZBFileCloseFunction handler)
	{
		m_NZBDrive->SetNZBFileCloseHandler(RelayEvent(handler));
	}
	void SetFileAddedHandler(NZBDokanDrive::FileAddedFunction handler)
	{
		m_NZBDrive->SetFileAddedHandler(RelayEvent(handler));
	}
	void SetFileInfoHandler(NZBDokanDrive::FileInfoFunction handler)
	{
		m_NZBDrive->SetFileInfoHandler(RelayEvent(handler));
	}
	void SetFileSegmentStateChangedHandler(NZBDokanDrive::FileSegmentStateChangedFunction handler)
	{
		m_NZBDrive->SetFileSegmentStateChangedHandler([handler](const int32_t fileID, const int32_t segment, const ByteFountain::SegmentState state)
		{
			SendEvent(handler, fileID, segment, adapt(state));
		});
	}
	void SetFileRemovedHandler(NZBDokanDrive::FileRemovedFunction handler)
	{
		m_NZBDrive->SetFileRemovedHandler(RelayEvent(handler));
	}
	void SetConnectionStateChangedHandler(NZBDokanDrive::ConnectionStateChangedFunction handler)
	{
		m_NZBDrive->SetConnectionStateChangedHandler([handler](const ByteFountain::ConnectionState state, const int32_t server, const int32_t thread)
		{
			SendEvent(handler, adapt(state), server, thread);
		});
	}
	void SetConnectionInfoHandler(NZBDokanDrive::ConnectionInfoFunction handler)
	{
		m_NZBDrive->SetConnectionInfoHandler(RelayEvent(handler));
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

	ULONG CopySecurityDescriptorTo(ULONG lengthSD, PSECURITY_DESCRIPTOR pSecDesc)
	{

		if (lengthSD >= m_securityDescriptorLength)
		{
			CopyMemory(pSecDesc, m_securityDescriptor.get(), m_securityDescriptorLength);
		}

		return m_securityDescriptorLength;
	}

	// IDokanContextStore:

	DokanContext& MakeContext()
	{
		std::unique_lock<std::mutex> lock(m_dokanContextsMutex);
		ULONG64 handle=++m_contextCounter;
		DokanContext& context=m_dokanContexts[handle];
		context.Handle=handle;
		return context;
	}

	std::shared_ptr<ByteFountain::IFile> GetFile(const ULONG64 handle)
	{
		std::unique_lock<std::mutex> lock(m_dokanContextsMutex);
		const auto itContext = m_dokanContexts.find(handle);
		if (itContext == m_dokanContexts.end()) return nullptr;
		return itContext->second.OpenFile;
	}
	std::shared_ptr<ByteFountain::IDirectory> GetDirectory(const ULONG64 handle)
	{
		std::unique_lock<std::mutex> lock(m_dokanContextsMutex);
		const auto itContext = m_dokanContexts.find(handle);
		if (itContext == m_dokanContexts.end()) return nullptr;
		return itContext->second.OpenDirectory;
	}

	ULONG64 MakeContext(std::shared_ptr<ByteFountain::IFile> file)
	{
		DokanContext& context(MakeContext());
		context.OpenFile=file;
		return context.Handle;
	}
	ULONG64 MakeContext(std::shared_ptr<ByteFountain::IDirectory> dir)
	{
		DokanContext& context(MakeContext());
		context.OpenDirectory=dir;
		return context.Handle;
	}
	void RemoveContext(ULONG64 handle)
	{
		std::unique_lock<std::mutex> lock(m_dokanContextsMutex);
		m_dokanContexts.erase(handle);
	}
	void ClearDokanContext()
	{
		std::unique_lock<std::mutex> lock(m_dokanContextsMutex);
		m_dokanContexts.clear();
	}
	std::shared_ptr<ByteFountain::IFile> GetFile(const std::filesystem::path& path)
	{
		return m_NZBDrive->GetFile(path);
	}
	std::shared_ptr<ByteFountain::IDirectory> GetDirectory(const std::filesystem::path& path)
	{
		return m_NZBDrive->GetDirectory(path);
	}
	std::shared_ptr<ByteFountain::IDirectory> GetRootDirectory()
	{
		return m_NZBDrive->GetRootDir();
	}
	unsigned long long GetTotalNumberOfBytes() const
	{
		return m_NZBDrive->GetTotalNumberOfBytes();
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
