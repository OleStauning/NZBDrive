/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef NZBDIRECTORY_HPP
#define NZBDIRECTORY_HPP

#include "IFile.hpp"
#include "IDirectory.hpp"
#include "ContentType.hpp"
#include <iostream>
#include <memory>
#include <map>
#include <boost/filesystem.hpp>
#include <thread>
#include <mutex>

namespace ByteFountain
{
	using namespace std;
	using namespace boost;
		
	class NZBDirectory : public IDirectory, public std::enable_shared_from_this< NZBDirectory >
	{
		std::mutex m_mutex;
		std::mutex& m_root_mutex;
		uint_fast64_t m_totalNumberOfBytes;
		
		NZBDirectory(std::mutex& root_mutex):m_root_mutex(root_mutex){}
		
		struct Element
		{
			std::shared_ptr<IFile> file;
			std::shared_ptr<NZBDirectory> dir;
			ContentType GetType() { return file.get() ? ContentType::File : ContentType::Directory; }
			~Element(){}
		};
		std::map< filesystem::path, Element > content;
		
		void RegisterFile(const std::shared_ptr<IFile>& file, filesystem::path::iterator i, const filesystem::path& p=filesystem::path());
		void Unmount(filesystem::path::iterator i);
		bool Exists(filesystem::path::iterator i);
		std::shared_ptr<NZBDirectory> GetDirectory(filesystem::path::iterator i);
		std::shared_ptr<IFile> GetFile(filesystem::path::iterator i);
		void MyEnumFiles(std::unique_lock<std::mutex>& lock, 
			std::function<void (const filesystem::path& path, std::shared_ptr<IFile> file)> callback,
			const filesystem::path& p=filesystem::path());
	public:
		NZBDirectory():m_mutex(),m_root_mutex(m_mutex){}
		~NZBDirectory();
		std::ostream& Print(std::ostream& ost, const filesystem::path& p=filesystem::path());
		void RegisterFile(const std::shared_ptr<IFile>& file, const filesystem::path& p);
		std::shared_ptr<IDirectory> GetDirectory(const filesystem::path& p);
		std::shared_ptr<IFile> GetFile(const filesystem::path& p);
		std::vector< std::pair<filesystem::path,ContentType> > GetContent();
		bool Exists(const filesystem::path& p);
		void Unmount(const filesystem::path& p);
		void Clear();
		void EnumFiles(std::function<void (const filesystem::path& path, std::shared_ptr<IFile> file)> callback,
			const filesystem::path& p=filesystem::path());
		uint_fast64_t GetTotalNumberOfBytes() const { return m_totalNumberOfBytes; }
	};
	

}
#endif




