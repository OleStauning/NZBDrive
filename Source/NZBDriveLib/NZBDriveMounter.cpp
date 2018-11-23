/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#include "NZBDriveMounter.hpp"
#include "NZBDriveIMPL.hpp"
#include "NZBDirectory.hpp"
#include "Logger.hpp"
#include "ReadAheadFile.hpp"

namespace ByteFountain
{

	template <typename T, typename... TArgs>
	static void SendEvent(T& handler, TArgs&&... args)
	{
		auto h = handler;
		if (h) h(std::forward<TArgs>(args)...);
	}

	NZBDriveMounter::NZBDriveMounter(NZBMountState& mount_state,
		NZBDriveIMPL& drv, const bool extract_archives,
		const boost::filesystem::path& dir, Logger& log,
		MountStatusFunction handler) :
		mstate(mount_state),
		state(mount_state.state),
		cancel(mount_state.cancel),
		nzbID(mount_state.nzbID),
		drive(drv),
		extract_archives(extract_archives),
		logger(log),
		mountdir(dir),
		mountStatusFunction(handler),
		split_file_factory(log,*this),
		rar_file_factory(log,*this),
		zip_file_factory(log,*this),
		finalizing(false),
		parts_loaded(0),
		parts_total(0),
		errors(0)
	{
		logger<<Logger::Debug<<"Mounting "<<mountdir<<Logger::End;
	}
		
	NZBDriveMounter::~NZBDriveMounter()
	{
		logger<<Logger::Info<<"Done mounting "<<mountdir<<Logger::End;
	}

	void NZBDriveMounter::StopInsertFile()
	{
		auto keep_this_alive = shared_from_this();
			
		parts_loaded++;

		if (!finalizing && parts_loaded==parts_total)
		{
			logger<<Logger::Debug<<"Finalizing"<<Logger::End;
			finalizing=true;
			split_file_factory.Finalize();
			rar_file_factory.Finalize();
			zip_file_factory.Finalize();
			finalizing=false;
			state = NZBMountState::Mounted;
		}
		if (!finalizing && state != NZBMountState::Canceling)
		{
			SendEvent(mountStatusFunction, nzbID, parts_loaded, parts_total);
		}
		if (parts_loaded==parts_total)
		{
			mountStatusFunction = nullptr;
			m_keep_this_alive.reset();
		}
	}

	void NZBDriveMounter::RawInsertFile(std::shared_ptr<InternalFile> file, const boost::filesystem::path& dir)
	{
		boost::filesystem::path filepath = dir / file->GetFileName();
			
		logger<<Logger::Debug<<"Registering file: "<<filepath<<Logger::End;

		errors|=file->GetErrorFlags();

		if (drive.m_root_dir->Exists(filepath))
		{
			logger<<Logger::Info<<"Already exists: "<<filepath<<Logger::End;
		}
		else
		{
			drive.m_root_dir->RegisterFile(file,filepath);
		}
	}

	void NZBDriveMounter::StartInsertFile(std::shared_ptr<InternalFile> file, const boost::filesystem::path& dir)
	{ 
		if (!m_keep_this_alive) m_keep_this_alive = shared_from_this();
		
		parts_total++;

		boost::filesystem::path filename=file->GetFileName();
			
		if (extract_archives && split_file_factory.AddFile(dir,file))
		{
			logger<<Logger::Debug<<"File was a split file: "<<dir/filename<<Logger::End;
			StopInsertFile();
		}
		else if (extract_archives && rar_file_factory.AddFile(dir,file,cancel))
		{
			logger<<Logger::Debug<<"File was a rar file: "<<dir/filename<<Logger::End;
		}
		else if (extract_archives && zip_file_factory.AddFile(dir,file,cancel))
		{
			logger<<Logger::Debug<<"File was a zip file: "<<dir/filename<<Logger::End;
		}
		else
		{
			filesystem::path filepath;
			
			if (file->IsPWProtected())
			{
				logger<<Logger::Warning<<"Adding '.pwprotected' to the name of password protected file: "
					<<dir/filename<<""<<Logger::End;

				filepath=dir/(filename.string() + ".pwprotected");
			}
			else if (file->IsCompressed())
			{
				logger<<Logger::Warning<<"Adding '.compressed' to the name of compressed file: "
					<<dir/filename<<""<<Logger::End;

				filepath=dir/(filename.string() + ".compressed");
			}
			else
			{
				filepath=dir/filename;
				logger<<Logger::Debug<<"Mounting file: "<<filepath<<Logger::End;
			}

			errors|=file->GetErrorFlags();

			if (drive.m_root_dir->Exists(filepath))
			{
				logger<<Logger::Debug<<"Already exists: "<<filepath<<Logger::End;
			}
			else
			{
				// Wrap file in read-ahead file:
				std::shared_ptr<IFile> rafile(new ReadAheadFile(drive.m_io_service,logger,file,drive));
				drive.m_root_dir->RegisterFile(rafile,filepath);
			}
			StopInsertFile();
		}	
	}
/*
	std::shared_ptr<IDriveMounter> NZBDriveMounter::GetSharedPtr()
	{
		return shared_from_this();
	}
*/	
}
