/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#include "NZBFuseDrive.hpp"
#include "IDirectory.hpp"
#include "IFile.hpp"
#include "Logger.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <functional>
#include <fstream>
#include <boost/filesystem.hpp>
#include <queue>
//#include <google/profiler.h>

using namespace ByteFountain;

std::string expand_user(std::string path) {
  if (not path.empty() and path[0] == '~') {
    assert(path.size() == 1 or path[1] == '/');  // or other error handling
    char const* home = getenv("HOME");
    if (home or ((home = getenv("USERPROFILE")))) {
      path.replace(0, 1, home);
    }
    else {
      char const *hdrive = getenv("HOMEDRIVE"),
        *hpath = getenv("HOMEPATH");
      assert(hdrive);  // or other error handling
      assert(hpath);
      path.replace(0, 1, std::string(hdrive) + hpath);
    }
  }
  return path;
}


bool parse_idx_param(const char* name, const std::string& param, size_t& idx)
{
	if (!boost::starts_with(param, name)) return false;
	
	try
	{
		idx = boost::lexical_cast<size_t>(param.substr(strlen(name)));
	}
	catch(const std::exception& e)
	{
		std::cout << "Invalid configuration parameter: "<<param<<std::endl;
		exit(1);
	}
	return true;
}



int main(int argc, char *argv[])
{ 
	if (argc < 2)
	{
		std::cout << "Usage: nzbmounter mount-point [nzbfile1, nzbfile2, ...]"<<std::endl;
		return 1;
	}
	
	std::string mountpoint(argv[1]);

	if (!boost::filesystem::exists(expand_user("~/.nzbmount")))
	{
		boost::filesystem::create_directory(expand_user("~/.nzbmount"));
	}
	if (!boost::filesystem::exists(expand_user("~/.nzbmount/config")))
	{
		std::ofstream cf(expand_user("~/.nzbmount/config"));
		cf<<"cache_keep=no"<<std::endl;
		cf<<"server=usenetserver.com"<<std::endl;
		cf<<"user=username"<<std::endl;
		cf<<"pass=password"<<std::endl;
		cf<<"connections=2"<<std::endl;
		cf<<"timeout=10"<<std::endl;
		cf<<"pipeline=4"<<std::endl;
		cf<<"pre_check_segments=no"<<std::endl;
		cf<<"logfile=no"<<std::endl;	
		cf.close();
		std::cout << "Edit the configuration file \"~/.nzbmount/config\" and try again"<<std::endl;
		return 1;
	}
	
	std::vector<UsenetServer> servers;

	LogLineLevel log_level = LogLineLevel::Debug;
	
	NetworkThrottling shape;;
	
	std::ifstream cf(expand_user("~/.nzbmount/config"));
	std::string line;
	while (std::getline(cf,line))
	{
		size_t idx=0;
		boost::algorithm::trim(line);
		if (line.size()<1 || line[0]=='#') continue;
		size_t pos=line.find('=');
		if (pos==std::string::npos)
		{
			std::cout << "Invalid configuration file: "<<line<<std::endl;
			return 1;
		}
		std::string param=line.substr(0,pos);
		std::string val=line.substr(pos+1);
		boost::algorithm::trim(param);
		boost::algorithm::trim(val);
		if (param=="cache_path")
		{
		}
		else if (param=="cache_keep") ;
		else if (param=="server" || parse_idx_param("server", param, idx))
		{
			servers.resize(idx+1);
			servers[idx].ServerName=val;
		}
		else if (param=="user" || parse_idx_param("user", param, idx))
		{
			servers.resize(idx+1);
			servers[idx].UserName=val;
		}
		else if (param=="pass" || parse_idx_param("pass", param, idx))
		{
			servers.resize(idx+1);
			servers[idx].Password=val;
		}
		else if (param=="port" || parse_idx_param("port", param, idx))
		{
			servers.resize(idx+1);
			servers[idx].Port=val;
		}
		else if (param=="use_ssl" || parse_idx_param("use_ssl", param, idx))
		{
			servers.resize(idx+1);
			servers[idx].UseSSL=boost::iequals(val, "yes")||boost::iequals(val, "y");
		}
		else if (param=="connections" || parse_idx_param("connections", param, idx))
		{
			try
			{
				servers.resize(idx+1);
				servers[idx].Connections=boost::lexical_cast<unsigned int>(val);
			}
			catch(const std::exception& e)
			{
				std::cout << "Invalid configuration file: "<<line<<std::endl;
				return 1;
			}
		}
		else if (param=="priority" || parse_idx_param("priority", param, idx))
		{
			try
			{
				servers.resize(idx+1);
				servers[idx].Priority=boost::lexical_cast<unsigned int>(val);
			}
			catch(const std::exception& e)
			{
				std::cout << "Invalid configuration file: "<<line<<std::endl;
				return 1;
			}
		}
		else if (param=="timeout" || parse_idx_param("timeout", param, idx))
		{
			try
			{
				servers.resize(idx+1);
				servers[idx].Timeout=boost::lexical_cast<unsigned int>(val);
			}
			catch(const std::exception& e)
			{
				std::cout << "Invalid configuration file: "<<line<<std::endl;
				return 1;
			}
		}
		else if (param=="pipeline" || parse_idx_param("pipeline", param, idx))
		{
			try
			{
				servers.resize(idx+1);
				servers[idx].Pipeline=boost::lexical_cast<unsigned int>(val);
			}
			catch(const std::exception& e)
			{
				std::cout << "Invalid configuration file: "<<line<<std::endl;
				return 1;
			}
		}
		else if (param=="log_file") ;
		else if (param=="log_level")
		{
			if (boost::iequals(val, "debug")) log_level=LogLineLevel::Debug;
			else if (boost::iequals(val, "warning")) log_level=LogLineLevel::Warning;
			else if (boost::iequals(val, "error")) log_level=LogLineLevel::Error;
			else if (boost::iequals(val, "fatal")) log_level=LogLineLevel::Fatal;
			else 
			{
				std::cout << "Invalid loglevel: "<<line<<std::endl;
				return 1;
			}
		}
		
		else if (param=="network_rate")
		{
			if (val=="constant")
			{
				shape.Mode = NetworkThrottling::Constant;
			}
			else if (val=="adaptive")
			{
				shape.Mode = NetworkThrottling::Adaptive;
				shape.AdaptiveIORatioPCT = 110;
				shape.FastPreCache = 10*1024*1024;
				shape.BackgroundNetworkRate = 1000;
			}
			else
			{
				std::cout << "Invalid configuration file: "<<line<<std::endl;
				return 1;
			}
		}
		else if (param=="network_limit")
		{
			try
			{
				shape.NetworkLimit = boost::lexical_cast<unsigned long>(val);
			}
			catch(const std::exception& e)
			{
				std::cout << "Invalid configuration file: "<<line<<std::endl;
				return 1;
			}
		}
		else
		{
			std::cout << "Invalid configuration file: "<<line<<std::endl;
			return 1;
		}
	}


	Logger log(std::cout,log_level);
	
	NZBFuseDrive drive(log);
	drive.SetNetworkThrottling(shape);
	
	try
	{
		drive.Start(mountpoint,servers);
	}
	catch(std::exception& e)
	{
		std::cout << "Failed to start nzbmounter: " << e.what() << std::endl;
		return 1;
	}
	
	std::function<void()> MountNext = nullptr;
	
	auto onMountStatus=[&](const int32_t nzbid, const int32_t parts_loaded, const int32_t parts_total)
	{
		int pct=100*parts_loaded/parts_total;
		std::cout<<"Parts loaded: "<<parts_loaded<<" / "<<parts_total<<" : "<<pct<<"%"<<std::endl;

		if (parts_loaded==parts_total)
		{
			std::cout<<"FILELIST:"<<std::endl;
			drive.EnumFiles([](const boost::filesystem::path& path, std::shared_ptr<IFile> file)
			{
				std::cout<<path;
				if (file->IsPWProtected())
				{
					std::cout<<" (FILE IS PASSWORD PROTECTED!!!!)";
				}
				if (file->IsCompressed())
				{
					std::cout<<" (FILE IS COMPRESSED)";
				}
				std::cout<<std::endl;
			});
			MountNext();
		}
	};

	std::string invalid_chars = ":\\/";
	
	auto MountDir=[&invalid_chars](std::string s)
	{
		std::replace_if(s.begin(), s.end(), 
                    [&invalid_chars](char c) { return invalid_chars.find(c)!=std::string::npos; }, '_');
		return boost::filesystem::path(s);
	};
	
	
	std::queue<std::string> nzbqueue;

	MountNext=[&]()
	{
		if (!nzbqueue.empty())
		{
			std::string nzbfile = nzbqueue.front();
			nzbqueue.pop();
			drive.Mount(MountDir(nzbfile),nzbfile,onMountStatus);
		}
	};

	for(int i=2;i<argc;++i) nzbqueue.push(argv[i]);
	MountNext();
	
	std::cout<<"Type NZB filename <enter> or quit <enter>"<<std::endl;

	std::string nzbfile;
	std::string input;
	do
	{
		std::cin.clear();
		getline (std::cin,input);
		boost::trim(input);
		if (input=="") 
		{
			std::cout<<"Enter filename or url to mount or \"quit\" to quit."<<std::endl;
			continue;
		}
		if (input=="quit") break;
		if (input=="cancel") { drive.Unmount(boost::filesystem::path(nzbfile).stem()); continue; }
		if (input=="dir")
		{
			continue;
		}
		nzbfile=input;
		drive.Mount(MountDir(nzbfile),nzbfile,onMountStatus);
	}
	while(true);

	
	std::cout<<"STOPPING...."<<std::endl;	
	
	drive.Stop();
	
	std::cout<<"BYE."<<std::endl;
	
	return 0;
} 
