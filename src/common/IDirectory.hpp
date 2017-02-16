/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef IDIRECTORY_H
#define IDIRECTORY_H

#include "ContentType.hpp"
#include "IFile.hpp"
#include <memory>
#include <vector>
#include <boost/filesystem.hpp>

namespace ByteFountain
{
	class IDirectory
	{
	public:
		virtual ~IDirectory() {};

		virtual std::shared_ptr<IDirectory> GetDirectory(const boost::filesystem::path& p) =0;
		virtual std::shared_ptr<IFile> GetFile(const boost::filesystem::path& p) = 0;
		virtual std::vector< std::pair<boost::filesystem::path, ContentType> > GetContent() = 0;
		virtual bool Exists(const boost::filesystem::path& p) = 0;
		virtual void EnumFiles(std::function<void(const boost::filesystem::path& path, std::shared_ptr<IFile> file)> callback,
			const boost::filesystem::path& p = boost::filesystem::path()) = 0;
		virtual uint_fast64_t GetTotalNumberOfBytes() const = 0;
	};
}

#endif
