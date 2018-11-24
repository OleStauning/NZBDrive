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
#include <filesystem>
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
		std::map< std::filesystem::path, Element > content;
		
		void RegisterFile(const std::shared_ptr<IFile>& file, std::filesystem::path::iterator i, const std::filesystem::path& p= std::filesystem::path());
		void Unmount(std::filesystem::path::iterator i);
		bool Exists(std::filesystem::path::iterator i);
		std::shared_ptr<NZBDirectory> GetDirectory(std::filesystem::path::iterator i);
		std::shared_ptr<IFile> GetFile(std::filesystem::path::iterator i);
		void MyEnumFiles(std::unique_lock<std::mutex>& lock, 
			std::function<void (const std::filesystem::path& path, std::shared_ptr<IFile> file)> callback,
			const std::filesystem::path& p= std::filesystem::path());
	public:
		NZBDirectory():m_mutex(),m_root_mutex(m_mutex){}
		~NZBDirectory();
		std::ostream& Print(std::ostream& ost, const std::filesystem::path& p= std::filesystem::path());
		void RegisterFile(const std::shared_ptr<IFile>& file, const std::filesystem::path& p);
		std::shared_ptr<IDirectory> GetDirectory(const std::filesystem::path& p);
		std::shared_ptr<IFile> GetFile(const std::filesystem::path& p);
		std::vector< std::pair<std::filesystem::path,ContentType> > GetContent();
		bool Exists(const std::filesystem::path& p);
		void Unmount(const std::filesystem::path& p);
		void Clear();
		void EnumFiles(std::function<void (const std::filesystem::path& path, std::shared_ptr<IFile> file)> callback,
			const std::filesystem::path& p= std::filesystem::path());
		uint_fast64_t GetTotalNumberOfBytes() const { return m_totalNumberOfBytes; }
	};
	

}
#endif




