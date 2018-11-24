/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef IFile_H
#define IFile_H

#include <iostream>
#include <string>
#include <functional>
#include <memory>
#include <boost/signals2.hpp>
#include <filesystem>

namespace ByteFountain
{
	class IFile
	{
	public:
		typedef boost::signals2::scoped_connection ConCancelScoped;
		typedef boost::signals2::signal<void ()> CancelSignal;

		virtual ~IFile(){};
		virtual bool IsCompressed() const=0;
		virtual bool IsPWProtected() const=0;
		virtual std::filesystem::path GetFileName()=0;
		virtual unsigned long long GetFileSize()=0;
		virtual bool GetFileData(char* buf, const unsigned long long offset, const std::size_t size, std::size_t& readsize) = 0;

		typedef std::function< void(const std::size_t readsize) > OnDataFunction;
		virtual void AsyncGetFileData(OnDataFunction func, char* buf, const unsigned long long offset, const std::size_t size,
			CancelSignal* cancel=0, const bool priority=false)=0;

		virtual void Unmount() = 0;
	};
}

#endif
