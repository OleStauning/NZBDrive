/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef NZB_HPP
#define NZB_HPP

#include <string>
#include <vector>
#include <set>
#include <map>
#include <boost/cstdint.hpp>
#include <boost/filesystem.hpp>

namespace ByteFountain
{
	struct nzb
	{
		struct file
		{
			struct segment
			{
				unsigned long number;
				unsigned long bytes;
				std::string message_id;
				segment():number(0),bytes(0),message_id(){}
				segment(const unsigned long n, const unsigned long b, const std::string& id):number(n),bytes(b),message_id(id){}
				std::ostream& print(std::ostream& ost) const
				{
					ost<<"             "<<number<<" "<<bytes<<" "<<message_id<<std::endl;
					return ost;
				}
			};
			std::set<std::string> groups;
			std::wstring poster;
			long date;
			unsigned long bytes;
			std::vector< segment > segments;

			// Parser information:
			std::string nzb_file_id;
			std::wstring subject;
			std::wstring filebase;
			std::wstring fileext;
			std::wstring filetype;
			unsigned int segments_found;
			unsigned int segments_total;

			// NZBDrive ydecoded information:
			boost::filesystem::path name;
			unsigned long long size;
			
			// Methods:
			bool has_data() const { return !segments.empty(); }
			
			file():
				groups(),
				poster(),
				date(),
				bytes(),
				segments(),
				nzb_file_id(),
				subject(),
				filebase(),
				fileext(),
				filetype(),
				segments_found(),
				segments_total(),
				name(),
				size()
			{}
			std::ostream& print(std::ostream& ost) const
			{
				for(const auto& s : segments)
				{
					ost<<"     Segment:"<<std::endl;
					s.print(ost);
				}
				return ost;
			}
		};
		std::vector<file> files;
		
		std::ostream& print(std::ostream& ost) const
		{
			for(const auto& f : files)
			{
				ost<<"File:"<<std::endl;
				f.print(ost);
			}
			return ost;
		}
	};
	
	struct nzb_ext : public nzb
	{
		long date;
		std::string nzb_id;
		std::wstring poster;
		std::wstring subject;
		std::wstring subject_pattern;
		unsigned long bytes;
		unsigned long parts_found;
		unsigned long parts_total;
		std::map<std::wstring,unsigned int> filecount;
		std::wstring nfo;
		bool has_pwinfo;
		bool has_pw;
		nzb_ext():
			date(0),
			poster(),
			subject(),
			subject_pattern(),
			bytes(),
			parts_found(0),
			parts_total(0),
			filecount(),
			nfo(),
			has_pwinfo(false),
			has_pw()
		{}
	};

	inline bool Validate(const nzb& n)
	{
		std::set<size_t> segments;
		for(const nzb::file& f : n.files)
		for(const nzb::file::segment& s : f.segments)
		{
			segments.insert(s.number);
		}
		size_t size=segments.size();
		return  size>0 && *segments.cbegin() == 1 && *segments.crbegin() == size ;
	}
	
	inline boost::uintmax_t Bytes(const nzb& n)
	{
		boost::uintmax_t bytes=0;
		std::set<size_t> segments;
		for(const nzb::file& f : n.files)
		for(const nzb::file::segment& s : f.segments)
		{
			if (segments.find(s.number) == segments.end())
			{
				bytes+=s.bytes;
				segments.insert(s.number);
			}
		}
		return bytes;
	}
	
}

#endif

