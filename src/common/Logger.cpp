/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#include "Logger.hpp"

namespace ByteFountain
{
#ifdef WIN32
#include <windows.h> 
#define red_col "SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED)"
#define green_col "SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN)"
#define brown_col "SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN)"
#define blue_col "SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_BLUE)"
#define magenta_col ""
#define cyan_col ""
#define gray_col ""

#define red_bold_col ""
#define green_bold_col ""
#define brown_bold_col ""
#define blue_bold_col ""
#define magenta_bold_col ""
#define cyan_bold_col ""
#define gray_bold_col ""
#define no_col "SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15)"
#else
#define red_col ost<<"\033[31m"
#define green_col ost<<"\033[32m"
#define brown_col ost<<"\033[33m"
#define blue_col ost<<"\033[34m"
#define magenta_col ost<<"\033[35m"
#define cyan_col ost<<"\033[36m"
#define gray_col ost<<"\033[37m"

#define red_bold_col ost<<"\033[31;1m"
#define green_bold_col ost<<"\033[32;1m"
#define brown_bold_col ost<<"\033[33;1m"
#define blue_bold_col ost<<"\033[34;1m"
#define magenta_bold_col ost<<"\033[35;1m"
#define cyan_bold_col ost<<"\033[36;1m"
#define gray_bold_col ost<<"\033[37;1m"
#define no_col ost<<"\033[0m"
#endif
	
Logger::Logger(std::ostream& ost, const LogLineLevel& level):
	m_callback([&ost](const LogLineLevel lvl, const std::string& msg)
		{
			switch(lvl)
			{
				case LogLineLevel::Debug: ost<<"Debug: "; break;
				case LogLineLevel::Info: ost<<"Info: "; break;
				case LogLineLevel::PopupInfo: ost<<"PopupInfo: "; break;
				case LogLineLevel::Warning: ost << "Warning: "; red_col; break;
				case LogLineLevel::Error: ost << "Error: "; red_bold_col; break;
				case LogLineLevel::PopupError: ost << "PopupError: "; red_bold_col; break;
				case LogLineLevel::Fatal: ost << "Fatal: "; red_bold_col; break;
			}
			ost << msg; no_col; ost << std::endl;
		}
	),m_level(level)
{}

Logger::Logger(OnLogLine callback, const LogLineLevel& level):m_callback(callback),m_level(level){}

void Logger::SetLoggingFunction(OnLogLine f) { m_callback = f; }
void Logger::SetLevel(const LogLineLevel& level){ m_level=level; }
LogLineLevel Logger::GetLevel(){ return m_level; }


const Logger::DebugType Logger::Debug;
const Logger::InfoType Logger::Info;
const Logger::PopupInfoType Logger::PopupInfo;
const Logger::WarningType Logger::Warning;
const Logger::ErrorType Logger::Error;
const Logger::PopupErrorType Logger::PopupError;
const Logger::FatalType Logger::Fatal;
const Logger::EndType Logger::End;

LogLine::LogLine(OnLogLine callback, const LogLineLevel& lvl, const bool enabled):m_callback(callback),m_ost(),m_level(lvl),m_enabled(enabled){}
LogLine::LogLine(const LogLine& el):m_callback(el.m_callback),m_ost(),m_level(el.m_level),m_enabled(el.m_enabled){}

LogLine& LogLine::operator<<(const Logger::EndType&) { if (m_enabled && m_callback) m_callback(m_level,m_ost.str()); return *this; }

LogLine Logger::operator<<(const Logger::DebugType&)
{
	return LogLine(m_callback,LogLineLevel::Debug,m_level<=LogLineLevel::Debug);
}
LogLine Logger::operator<<(const Logger::InfoType&)
{
	return LogLine(m_callback,LogLineLevel::Info,m_level<=LogLineLevel::Info);
}
LogLine Logger::operator<<(const Logger::PopupInfoType&)
{
	return LogLine(m_callback,LogLineLevel::PopupInfo,m_level<=LogLineLevel::PopupInfo);
}
LogLine Logger::operator<<(const Logger::WarningType&)
{
	return LogLine(m_callback,LogLineLevel::Warning,m_level<=LogLineLevel::Warning);
}
LogLine Logger::operator<<(const Logger::ErrorType&)
{
	return LogLine(m_callback,LogLineLevel::Error,m_level<=LogLineLevel::Error);
}
LogLine Logger::operator<<(const Logger::PopupErrorType&)
{
	return LogLine(m_callback,LogLineLevel::PopupError,m_level<=LogLineLevel::PopupError);
}
LogLine Logger::operator<<(const Logger::FatalType&)
{
	return LogLine(m_callback,LogLineLevel::Fatal,m_level<=LogLineLevel::Fatal);
}

}
