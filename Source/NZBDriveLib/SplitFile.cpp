/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#include "SplitFile.hpp"
#include "assert.h"
#include <boost/xpressive/xpressive.hpp>
#include <string>
#include <atomic>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace ByteFountain
{

using namespace boost;	
using namespace boost::xpressive;
	
	SplitFile::SplitFile(Logger& log, const std::string& name):
		MultipartFile(log, ".", name)
	{}
	SplitFile::~SplitFile()
	{
	}
	SplitFileFactory::SplitFileFactory(Logger& log, IDriveMounter& mounter):m_log(log),m_mounter(mounter){}
	
	bool SplitFileFactory::GetSplitFilenameAndPart(std::shared_ptr<InternalFile> file, std::string& name, int& part)
	{
		std::filesystem::path filename=file->GetFileName();
	
		std::string ext;
		smatch what;
		
		static sregex splitfile = sregex::compile("(?i:^(.*?)\\.(\\d+)$)");
		
		std::string strFilename(filename.string());

		if (regex_match( strFilename, what, splitfile ))
		{
			name=what[1];
			ext=what[2];
			try
			{
				part=std::stoi(ext);
			}
			catch(std::out_of_range& /*e*/)
			{
				return false;
			}
			catch(std::exception& /*e*/)
			{
				m_log<<Logger::Error<<"Failed to parse integer from "<<ext<<" in GetSplitFilenameAndPart, filename="<<filename<<Logger::End;
				throw;
			}
			return true;
		}
		return false;
	}

	bool SplitFileFactory::AddFile(const std::filesystem::path& path, std::shared_ptr<InternalFile> file)
	{
		std::string name;
		int part=0;
		if (!GetSplitFilenameAndPart(file,name,part)) return false;
		
		std::pair<std::filesystem::path,std::string> key=std::make_pair(path,name);
		
		SplitFileParts& parts=m_split_file_parts[key];

		parts.path = path;
		parts.name = name;
		parts.parts[part] = file;
		
		return true;
	}

	void SplitFileFactory::Finalize()
	{
		for(auto& parts : m_split_file_parts)
		{
			std::shared_ptr<SplitFile> splitFile(new SplitFile(m_log,parts.second.name));
			size_t count=0;
			int lastpart;
			bool complete=true;
			for(auto& part : parts.second.parts)
			{
				count++;
				bool bof = count==1;
				bool eof = count==parts.second.parts.size();

				if (!bof && lastpart+1 != part.first) complete=false;
				lastpart=part.first;

				splitFile->SetFilePart(part.first,part.second,0,part.second->GetFileSize(),bof,eof);
			}

//			splitFile->Finalize();

			if (complete) // Only register split-file if all parts are present:
			{
				m_mounter.StartInsertFile(splitFile,parts.second.path);
			}
			else
			{
				// TODO: Register the individual parts instead...
			}

		}

		m_split_file_parts.clear();
	}
	
}
