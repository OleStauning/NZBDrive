/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef NZBDRIVEMOUNTER_HPP
#define NZBDRIVEMOUNTER_HPP

#include "IDriveMounter.hpp"
#include "NZBDrive.hpp"
#include "SplitFile.hpp"
#include "RARFile.hpp"
#include "ZIPFile.hpp"

#include <boost/filesystem.hpp>

namespace ByteFountain
{
	class Logger;
	class NZBDriveIMPL;

	struct NZBDriveMounter : public IDriveMounter, std::enable_shared_from_this<NZBDriveMounter> 
	{
		typedef boost::signals2::signal<void()> CancelSignal;
		enum State { Mounting, Mounted, Canceling };
		State state;
		NZBDriveIMPL& drive;
		Logger& logger;
		int32_t nzbID;
		const boost::filesystem::path mountdir;
		MountStatusFunction mountStatusFunction;
		SplitFileFactory split_file_factory;
		RARFileFactory rar_file_factory;
		ZIPFileFactory zip_file_factory;
		bool finalizing;
		int parts_loaded;
		int parts_total;
//		bool canceling;
		CancelSignal cancel;
		FileErrorFlags errors;
		
		NZBDriveMounter(NZBDriveIMPL& drv, const int32_t id, const boost::filesystem::path& dir, Logger& log,
			MountStatusFunction handler);
		~NZBDriveMounter();
		void StopInsertFile();
		void RawInsertFile(std::shared_ptr<InternalFile> file, const boost::filesystem::path& dir);
		void StartInsertFile(std::shared_ptr<InternalFile> file, const boost::filesystem::path& dir);
		std::shared_ptr<IDriveMounter> GetSharedPtr();
		void Cancel();
	};
	
}

#endif