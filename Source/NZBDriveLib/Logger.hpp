/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef LOGGER_HPP
#define LOGGER_HPP
#include "LogLineLevel.hpp"
#include <sstream>
#include <string>
#include <functional>

namespace ByteFountain
{

class LogLine;

typedef std::function<void(const LogLineLevel lvl, const std::string& msg)> OnLogLine;

class Logger
{
	OnLogLine m_callback;
	LogLineLevel m_level;
	Logger(const Logger&) = delete;
public:
	Logger(std::ostream& ost, const LogLineLevel& level = LogLineLevel::Debug);
	Logger(OnLogLine callback, const LogLineLevel& level = LogLineLevel::Debug);
	
	void SetLoggingFunction(OnLogLine f);
	void SetLevel(const LogLineLevel& level);
	LogLineLevel GetLevel();
	
	class DebugType {};
	class InfoType {};
	class PopupInfoType {};
	class WarningType {};
	class ErrorType {};
	class PopupErrorType {};
	class FatalType {};
	class EndType {};
	
	static const DebugType Debug;
	static const InfoType Info;
	static const PopupInfoType PopupInfo;
	static const WarningType Warning;
	static const ErrorType Error;
	static const PopupErrorType PopupError;
	static const FatalType Fatal;
	static const EndType End;
	
	LogLine operator<<(const DebugType& info);
	LogLine operator<<(const InfoType& info);
	LogLine operator<<(const PopupInfoType& info);
	LogLine operator<<(const WarningType& info);
	LogLine operator<<(const ErrorType& info);
	LogLine operator<<(const PopupErrorType& info);
	LogLine operator<<(const FatalType& info);
};

class LogLine
{
	OnLogLine m_callback;
	std::ostringstream m_ost;
	LogLineLevel m_level;
	bool m_enabled;
public:
	LogLine(OnLogLine callback, const LogLineLevel& lvl, const bool enabled);
	LogLine(const LogLine& el);

	template <typename T>
	LogLine& operator<<(const T& val) { if (m_enabled) m_ost<<val; return *this; }

	LogLine& operator<<(const Logger::EndType&);
};

}

#endif
