/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef NZBFUSE_DRIVE
#define NZBFUSE_DRIVE

#include "NZBDrive.hpp"
#include "Logger.hpp"
#include <boost/filesystem.hpp>
#include <thread>
#include <string>
#include <mutex>
#include <condition_variable>

struct fuse;
struct fuse_chan;

namespace ByteFountain
{

class NZBFuseDrive : public NZBDrive
{
	fuse* m_fuse;
	fuse_chan* m_ch;
	boost::filesystem::path m_cache_path;
	std::string m_drivepath;
	std::thread m_fuseThread;
	bool m_fuseThreadRunning;
	std::mutex m_fuseThreadRunningMutex;
	std::condition_variable m_fuseThreadRunningConditionVariable;
	Logger& m_log;
	
	void FusermountCmd();
public:
	NZBFuseDrive(Logger& log);
	void Start(const std::string& drivepath, const std::vector<UsenetServer>& servers);
	virtual void Stop();
	void StopNow();

};

}

#endif
