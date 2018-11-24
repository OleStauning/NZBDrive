/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef NZBDRIVEMOUNTER_HPP
#define NZBDRIVEMOUNTER_HPP

#include "IDriveMounter.hpp"
#include "NZBMountState.hpp"
#include "NZBDrive.hpp"
#include "SplitFile.hpp"
#include "RARFile.hpp"
#include "ZIPFile.hpp"

#include <filesystem>

namespace ByteFountain
{
	class Logger;
	class NZBDriveIMPL;

	struct NZBDriveMounter : public IDriveMounter, std::enable_shared_from_this<NZBDriveMounter> 
	{
		NZBMountState& mstate;
		NZBMountState::State& state;
		NZBMountState::CancelSignal& cancel;
		int32_t nzbID;
		NZBDriveIMPL& drive;
		Logger& logger;
		const std::filesystem::path mountdir;
		MountStatusFunction mountStatusFunction;
		SplitFileFactory split_file_factory;
		RARFileFactory rar_file_factory;
		ZIPFileFactory zip_file_factory;
		bool finalizing;
		int parts_loaded;
		int parts_total;
		FileErrorFlags errors;
		bool extract_archives;
		std::shared_ptr<NZBDriveMounter> m_keep_this_alive;
		
		NZBDriveMounter(NZBMountState& mount_state,
			NZBDriveIMPL& drv, const bool extract_archives, 
			const std::filesystem::path& dir, 
			Logger& log, MountStatusFunction handler);
		~NZBDriveMounter();
		void StopInsertFile();
		void RawInsertFile(std::shared_ptr<InternalFile> file, const std::filesystem::path& dir);
		void StartInsertFile(std::shared_ptr<InternalFile> file, const std::filesystem::path& dir);
//		std::shared_ptr<IDriveMounter> GetSharedPtr();
	};
	
}

#endif
