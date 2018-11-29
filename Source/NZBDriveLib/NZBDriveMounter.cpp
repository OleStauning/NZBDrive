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

	NZBDriveMounter::NZBDriveMounter(
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
		bool extract_archives):
			m_io_service(io_service),
			m_drive(drive),
			m_logger(logger),
			m_clients(clients),
			m_segment_cache(segment_cache),
			m_root_dir(root_dir),
			m_root_rawdir(root_rawdir),
			m_fileAddedFunction(fileAddedFunction),
			m_fileInfoFunction(fileInfoFunction),
			m_fileSegmentStateChangedFunction(fileSegmentStateChangedFunction),
			m_fileRemovedFunction(fileRemovedFunction),
			m_mountStatusFunction(mountStatusFunction),
			m_mstate(mstate),
			m_state(mstate.state),
			m_cancel(mstate.cancel),
			m_nzbID(mstate.nzbID),
			m_mountdir(mountdir),
			m_split_file_factory(logger, *this),
			m_rar_file_factory(logger, *this),
			m_zip_file_factory(logger, *this),
			m_parts_loaded(0),
			m_parts_total(0),
			m_errors(0),
			m_extract_archives(extract_archives)
	{
		m_logger<<Logger::Debug<<"Mounting "<<m_mountdir<<Logger::End;
	}


	void NZBDriveMounter::PartIdentified()
	{
		m_parts_total++;
	}
	void NZBDriveMounter::PartFinalized()
	{
		m_parts_loaded++;
		SendEvent(m_mountStatusFunction, m_nzbID, m_parts_loaded, m_parts_total);
	}
	
	NZBDriveMounterScope NZBDriveMounter::NewPartScope()
	{
		return NZBDriveMounterScope(shared_from_this());
	}
	
		
	NZBDriveMounter::~NZBDriveMounter()
	{
		m_split_file_factory.Finalize();
		m_rar_file_factory.Finalize();
		m_zip_file_factory.Finalize();
		m_state = NZBMountState::Mounted;

		if (m_state != NZBMountState::Canceling)
		{
			SendEvent(m_mountStatusFunction, m_nzbID, m_parts_loaded, m_parts_total);
		}

		m_logger<<Logger::Info<<"Done mounting "<<m_mountdir<<Logger::End;
	}

	void NZBDriveMounter::RawInsertFile(std::shared_ptr<InternalFile> file, const std::filesystem::path& dir)
	{
		std::filesystem::path filepath = dir / file->GetFileName();
			
		m_logger<<Logger::Debug<<"Registering file: "<<filepath<<Logger::End;

		m_errors|=file->GetErrorFlags();

		if (m_root_dir->Exists(filepath))
		{
			m_logger<<Logger::Info<<"Already exists: "<<filepath<<Logger::End;
		}
		else
		{
			m_root_dir->RegisterFile(file,filepath);
		}
	}

	void NZBDriveMounter::StartInsertFile(std::shared_ptr<InternalFile> file, const std::filesystem::path& dir)
	{ 
		std::filesystem::path filename=file->GetFileName();
			
		if (m_extract_archives && m_split_file_factory.AddFile(dir,file))
		{
			m_logger<<Logger::Debug<<"File was a split file: "<<dir/filename<<Logger::End;
		}
		else if (m_extract_archives && m_rar_file_factory.AddFile(dir,file,m_cancel))
		{
			m_logger<<Logger::Debug<<"File was a rar file: "<<dir/filename<<Logger::End;
		}
		else if (m_extract_archives && m_zip_file_factory.AddFile(dir,file,m_cancel))
		{
			m_logger<<Logger::Debug<<"File was a zip file: "<<dir/filename<<Logger::End;
		}
		else
		{
			std::filesystem::path filepath;
			
			if (file->IsPWProtected())
			{
				m_logger<<Logger::Warning<<"Adding '.pwprotected' to the name of password protected file: "
					<<dir/filename<<""<<Logger::End;

				filepath=dir/(filename.string() + ".pwprotected");
			}
			else if (file->IsCompressed())
			{
				m_logger<<Logger::Warning<<"Adding '.compressed' to the name of compressed file: "
					<<dir/filename<<""<<Logger::End;

				filepath=dir/(filename.string() + ".compressed");
			}
			else
			{
				filepath=dir/filename;
				m_logger<<Logger::Debug<<"Mounting file: "<<filepath<<Logger::End;
			}

			m_errors|=file->GetErrorFlags();

			if (m_root_dir->Exists(filepath))
			{
				m_logger<<Logger::Debug<<"Already exists: "<<filepath<<Logger::End;
			}
			else
			{
				// Wrap file in read-ahead file:
				std::shared_ptr<IFile> rafile(new ReadAheadFile(m_io_service,m_logger,file,m_drive));
				m_root_dir->RegisterFile(rafile,filepath);
			}
		}	
	}
	
	void NZBDriveMounter::Mount(const nzb& mountfile)
	{
		for (const nzb::file& nzbfile : mountfile.files)
		{
			int32_t fileID = m_drive.m_fileCount++;

			std::shared_ptr<NZBFile> file(
				new NZBFile(m_io_service, m_nzbID, fileID, nzbfile, m_clients, m_segment_cache, m_logger,
				m_fileAddedFunction, m_fileInfoFunction, m_fileSegmentStateChangedFunction, m_fileRemovedFunction)
				);

//			m_log << Logger::Info << "AsyncGetFilename Called: " << file << Logger::End;
			
			file->AsyncGetFilename(
				[file, this, scope = NewPartScope()](const std::filesystem::path& filename)
			{
//				m_log << Logger::Info << "AsyncGetFilename Returns: " << file << Logger::End;

				if (0 != ((FileErrorFlags)file->GetErrorFlags() & (FileErrorFlags)FileErrorFlag::CacheFileError))
				{
					if (m_state == NZBMountState::Mounting)
					{
						m_logger << Logger::PopupError << "Failed to allocate cache-file. Mounting of " << m_mountdir << " will not succeed.\nPlease free some disk-space and try again." << Logger::End;
					}
				}
				else if (filename.empty())
				{
					for (int n = 1; true; ++n)
					{
						std::ostringstream oss;
						oss << "ErrorFile" << n;
						if (!m_root_dir->Exists(m_mountdir / oss.str()))
						{
							m_logger << Logger::Error << "Could not read filename, using filename " <<
								oss.str() << " instead" << Logger::End;
							m_errors++;
							StartInsertFile(file, m_mountdir / oss.str());
							break;
						}
					}
				}
				else
				{
					try
					{
						m_root_rawdir->RegisterFile(file, m_mountdir / filename);
					}
					catch (std::exception& e)
					{
						m_logger << Logger::Error << "Error when registering file: " << e.what() << Logger::End;
						m_errors++;
					}
					StartInsertFile(file, m_mountdir);
				}
//				m_logger << Logger::Info <<  drive_mounter.use_count() << Logger::End;
			}, 
			&m_cancel);
		}

	}
}
