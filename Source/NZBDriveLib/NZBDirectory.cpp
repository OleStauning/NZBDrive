/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#include "NZBDirectory.hpp"

namespace ByteFountain
{
	using namespace std;
	using namespace boost;
	
	void NZBDirectory::RegisterFile(const std::shared_ptr<IFile>& file, filesystem::path::iterator i, const filesystem::path& p)
	{
		Element& e(content[*i]);
		i++;
	
		if (e.file || (i->empty()&&e.dir)) throw std::invalid_argument("File or path already exists");
		if (i->empty()) e.file=file;
		else
		{
			if (!e.dir) e.dir.reset(new NZBDirectory(m_root_mutex));
			e.dir->RegisterFile(file,i);
		}
	}
	void NZBDirectory::Unmount(filesystem::path::iterator i)
	{
		if (i->string()=="/") i++; // TODO: IMPROVE THIS??
		if (i->empty())
		{ 
			for (const auto& element : content)
			{
				const Element& e(element.second);
				if (e.file) e.file->Unmount();
				if (e.dir) e.dir->Unmount(i);
			}
		}
		else
		{
			const auto ci = content.find(*i);
			if (ci == content.end()) return; // not found...

			const Element& e(ci->second);
			i++;
			if (i->empty())
			{
				Unmount(i);
				content.erase(ci);
			}
			else
			{
				if (e.dir) e.dir->Unmount(i);
			}
		}
	}
	void NZBDirectory::Clear()
	{
		std::unique_lock<std::mutex> lock(m_root_mutex);
		content.clear();
	}
	bool NZBDirectory::Exists(filesystem::path::iterator i)
	{
		if (i->string()=="/") i++; // TODO: IMPROVE THIS??
		if (i->empty()) return false;
		std::map< filesystem::path, Element >::const_iterator ci=content.find(*i);
		if (ci==content.end()) return false;
		const Element& e(ci->second);
		i++;
		if (i->empty()) return true;
		else if (e.dir) return e.dir->Exists(i);
		return false;
	}
	std::shared_ptr<NZBDirectory> NZBDirectory::GetDirectory(filesystem::path::iterator i)
	{
		if (i->string()=="/") i++; // TODO: IMPROVE THIS??
		if (i->empty()) return shared_from_this();
		std::map< filesystem::path, Element >::const_iterator ci=content.find(*i);
		if (ci==content.end()) return std::shared_ptr<NZBDirectory>();
		const Element& e(ci->second);
		i++;
		if (!e.dir) return e.dir;
		return e.dir->GetDirectory(i);
	}
	std::shared_ptr<IFile> NZBDirectory::GetFile(filesystem::path::iterator i)
	{
		if (i->string()=="/") i++; // TODO: IMPROVE THIS??
		if (i->empty()) return std::shared_ptr<IFile>();
		std::map< filesystem::path, Element >::const_iterator ci=content.find(*i);
		if (ci==content.end()) return std::shared_ptr<IFile>();
		const Element& e(ci->second);
		i++;
		if (i->empty()) return e.file;
		if (!e.dir) return std::shared_ptr<IFile>();
		return e.dir->GetFile(i);
	}
	void NZBDirectory::MyEnumFiles(std::unique_lock<std::mutex>& lock,
		std::function<void (const filesystem::path& path, std::shared_ptr<IFile> file)> callback,
		const filesystem::path& p)
	{
		for(auto& i : content)
		{
			filesystem::path p1=p/i.first;
			Element e=i.second;
			if (e.file)
			{
				lock.unlock();
				callback(p1,e.file);
				lock.lock();
			}
			else if (e.dir) e.dir->MyEnumFiles(lock,callback,p1);
		}
	}
	void NZBDirectory::RegisterFile(const std::shared_ptr<IFile>& file, const filesystem::path& p)
	{
		std::cout<<"Registering file: "<<p<<std::endl;
		
		std::unique_lock<std::mutex> lock(m_root_mutex);
		m_totalNumberOfBytes+=file->GetFileSize();
		RegisterFile(file,p.begin());
	}
	std::shared_ptr<IDirectory> NZBDirectory::GetDirectory(const filesystem::path& p)
	{
		std::unique_lock<std::mutex> lock(m_root_mutex);
		return GetDirectory(p.begin()); 
	}
	std::shared_ptr<IFile> NZBDirectory::GetFile(const filesystem::path& p)
	{ 
		std::unique_lock<std::mutex> lock(m_root_mutex);
		return GetFile(p.begin()); 
	}
	void NZBDirectory::Unmount(const filesystem::path& p) 
	{ 
		std::unique_lock<std::mutex> lock(m_root_mutex);
		Unmount(p.begin());
	}
	bool NZBDirectory::Exists(const filesystem::path& p) 
	{ 
		std::unique_lock<std::mutex> lock(m_root_mutex);
		return Exists(p.begin()); 
	}
	std::vector< std::pair<filesystem::path,ContentType> > NZBDirectory::GetContent()
	{
		std::unique_lock<std::mutex> lock(m_root_mutex);
		std::vector< std::pair<filesystem::path,ContentType> > res;
		for(auto e : content)
		{
			res.push_back(std::make_pair(e.first,e.second.GetType()));
		}
		return res;
	}
	void NZBDirectory::EnumFiles(std::function<void (const filesystem::path& path, std::shared_ptr<IFile> file)> callback,
		const filesystem::path& p) 
	{
		std::unique_lock<std::mutex> lock(m_root_mutex);
		MyEnumFiles(lock,callback,p);
	}
	std::ostream& NZBDirectory::Print(std::ostream& ost, const filesystem::path& p)
	{
		std::unique_lock<std::mutex> lock(m_root_mutex);
		MyEnumFiles(lock, [&ost](const filesystem::path& path, std::shared_ptr<IFile> file)
			{
				ost<<path<<std::endl;
			}
			,"/"
		);
		return ost;
	}
	NZBDirectory::~NZBDirectory()
	{
		for(auto& element : content)
		{
			if (element.second.file.use_count()>1)
			{
//				std::cout<<"Dangling file."<<std::endl;
				element.second.file.reset();
			}
			if (element.second.dir.use_count()>1)
			{
//				std::cout<<"Dangling dir."<<std::endl;
				element.second.dir.reset();
			}
		}
	}


}
