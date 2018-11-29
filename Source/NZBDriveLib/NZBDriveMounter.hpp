/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef NZBDRIVEMOUNTER_HPP
#define NZBDRIVEMOUNTER_HPP

#include <boost/asio.hpp>

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
	class SegmentCache;
	class NZBDirectory;
	class NewsClientCache;

	struct NZBDriveMounter : public IDriveMounter, std::enable_shared_from_this<NZBDriveMounter> 
	{
		boost::asio::io_service& m_io_service;
		NZBDriveIMPL& m_drive;
		Logger& m_logger;
		NewsClientCache& m_clients;
		SegmentCache& m_segment_cache;
		std::shared_ptr<NZBDirectory> m_root_dir;
		std::shared_ptr<NZBDirectory> m_root_rawdir;
		NZBFileAddedFunction m_fileAddedFunction;
		NZBFileInfoFunction m_fileInfoFunction;
		NZBFileSegmentStateChangedFunction m_fileSegmentStateChangedFunction;
		NZBFileRemovedFunction m_fileRemovedFunction;
		MountStatusFunction m_mountStatusFunction;
		NZBMountState& m_mstate;
		NZBMountState::State& m_state;
		NZBMountState::CancelSignal& m_cancel;
		int32_t m_nzbID;
		const std::filesystem::path m_mountdir;
		SplitFileFactory m_split_file_factory;
		RARFileFactory m_rar_file_factory;
		ZIPFileFactory m_zip_file_factory;
		int m_parts_loaded;
		int m_parts_total;
		FileErrorFlags m_errors;
		bool m_extract_archives;
		
		NZBDriveMounter(
			boost::asio::io_service& io_service,
			NZBDriveIMPL& drive,
			Logger& logger,
			NewsClientCache& clients,
			SegmentCache& segment_cache,
			std::shared_ptr<NZBDirectory>& root_dir,
			std::shared_ptr<NZBDirectory>& root_rawdir,
			NZBFileAddedFunction fileAddedFunction,
			NZBFileInfoFunction fileInfoFunction,
			NZBFileSegmentStateChangedFunction fileSegmentStateChangedFunction,
			NZBFileRemovedFunction fileRemovedFunction,
			MountStatusFunction mountStatusFunction,
			NZBMountState& mstate,
			const std::filesystem::path mountdir,
			bool extract_archives);
		
		~NZBDriveMounter();
		void PartIdentified();
		void PartFinalized();
		void RawInsertFile(std::shared_ptr<InternalFile> file, const std::filesystem::path& dir);
		void StartInsertFile(std::shared_ptr<InternalFile> file, const std::filesystem::path& dir);
		void Mount(const nzb& mountfile);
		NZBDriveMounterScope NewPartScope();
	};
	
}

#endif
