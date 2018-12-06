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
	std::string m_drivepath;
	std::thread m_fuseThread;
	bool m_fuseThreadRunning;
	std::mutex m_fuseThreadRunningMutex;
	std::condition_variable m_fuseThreadRunningConditionVariable;
	Logger& m_log;
	
	void StartFuse();
	void StopFuse();
public:
	NZBFuseDrive(Logger& log);
	void Start(const std::string& drivepath, const std::vector<UsenetServer>& servers);
	void SetDrivePath(const std::string& drivepath);
	virtual void Stop();

};

}

#endif
