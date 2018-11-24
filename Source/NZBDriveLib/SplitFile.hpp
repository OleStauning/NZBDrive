/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef SPLIT_FILE
#define SPLIT_FILE
#include "MultipartFile.hpp"
#include "IDriveMounter.hpp"
#include "Logger.hpp"
#include <filesystem>
#include <map>
#include <memory>

namespace ByteFountain
{
	
class SplitFile : public MultipartFile
{
	
public:
	SplitFile(const SplitFile&) = delete;
	SplitFile& operator=(const SplitFile&) = delete;

	SplitFile(Logger& log, const std::string& name);
	virtual ~SplitFile();
};
	
	
class SplitFileFactory
{
	Logger& m_log;
	IDriveMounter& m_mounter;

	struct SplitFileParts
	{
		typedef std::map<int, std::shared_ptr<InternalFile> > PartsMap;
		std::filesystem::path path;
		std::string name;
		PartsMap parts;
	};

	std::map<std::pair<std::filesystem::path,std::string>, SplitFileParts > m_split_file_parts;

	bool GetSplitFilenameAndPart(std::shared_ptr<InternalFile> file, std::string& name, int& part);

public:
	SplitFileFactory(Logger& log, IDriveMounter& mounter);
		
	bool AddFile(const std::filesystem::path& path, std::shared_ptr<InternalFile> file);
	void Finalize();
};

}
#endif
