/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#include "nzb.hpp"
#include "nzb_fileloader.hpp"
#include "text_tool.hpp"
#include <tinyxml2.h>
#include <boost/lexical_cast.hpp>
#include <filesystem>

#include <sstream>
#include <iostream>
/*
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
*/
/*
#define CURL_STATICLIB 
#include <stdio.h>
#include <curl/curl.h>
*/
#include "HTTPDownloader.hpp"
#include <boost/regex.hpp>


namespace ByteFountain
{
//	using namespace tinyxml2;
//	using namespace boost::network;
	
	boost::regex expr_http{"(?i)http(s?):\\/\\/((?:www\\.)?[-a-zA-Z0-9@:%._\\+~#=]{2,256}\\.[a-z]{2,6})(?::(\\d+))?\\b([-a-zA-Z0-9@:%_\\+.~#?&//=]*)$"};

	nzb loadnzb(const std::string& nzbfile, tinyxml2::XMLDocument& doc)
	{

		tinyxml2::XMLHandle hDoc(&doc);
		tinyxml2::XMLElement* pElem;
		tinyxml2::XMLHandle hRoot(0);

		// block: nzb
		{
			pElem=hDoc.FirstChildElement().ToElement();
			// should always have a valid root but handle gracefully if it does
			if (!pElem || pElem->Value()!=std::string("nzb") )
			{
				std::ostringstream oss;
				oss<<"Invalid nzb file: "<<nzbfile<<std::endl;
				throw std::invalid_argument(oss.str());
			}
			// save this for later
			hRoot= tinyxml2::XMLHandle(pElem);
		}

		ByteFountain::nzb nzb;

		tinyxml2::XMLElement* pElemFile=hRoot.FirstChildElement( "file" ).ToElement();
		if (!pElemFile)
		{
			std::ostringstream oss;
			oss<<"Invalid nzb file (no file-tag): "<<nzbfile<<std::endl;
			throw std::invalid_argument(oss.str());
		}
		size_t nfiles=0;
		for(/*nothing*/; pElemFile; pElemFile=pElemFile->NextSiblingElement()) ++nfiles;

		pElemFile=hRoot.FirstChildElement( "file" ).ToElement();
		nzb.files.resize(nfiles);
		size_t i=0;
		for(/*nothing*/; pElemFile; pElemFile=pElemFile->NextSiblingElement())
		{
			ByteFountain::nzb::file& file=nzb.files[i++];
			const char* val;
			val=pElemFile->Attribute("poster");
			if (val!=0) file.poster=ByteFountain::text_tool::utf8_to_unicode(std::string(val));
			val=pElemFile->Attribute("subject");
			if (val!=0) file.subject=ByteFountain::text_tool::utf8_to_unicode(std::string(val));
			val=pElemFile->Attribute("date");
			if (val!=0) file.date=boost::lexical_cast<long>(val);

			tinyxml2::XMLElement* pElemGroup= tinyxml2::XMLHandle(pElemFile).FirstChildElement( "groups" ).FirstChildElement( "group" ).ToElement();
			if (!pElemGroup)
			{
				std::ostringstream oss;
				oss<<"Invalid nzb file (no group-tag): "<<nzbfile<<std::endl;
				throw std::invalid_argument(oss.str());
			}
			for(/*nothing*/; pElemGroup; pElemGroup=pElemGroup->NextSiblingElement())
			{
				file.groups.insert(pElemGroup->GetText());
			}

			tinyxml2::XMLElement* pElemSegment=tinyxml2::XMLHandle(pElemFile).FirstChildElement( "segments" ).FirstChildElement( "segment" ).ToElement();
			if (!pElemSegment)
			{
				std::ostringstream oss;
				oss<<"Invalid nzb file (no segment-tag): "<<nzbfile<<std::endl;
				throw std::invalid_argument(oss.str());
			}
			for(/*nothing*/; pElemSegment; pElemSegment=pElemSegment->NextSiblingElement())
			{
				ByteFountain::nzb::file::segment segment;
				segment.message_id=pElemSegment->GetText();
				val=pElemSegment->Attribute("bytes");
				if (val!=0) segment.bytes=boost::lexical_cast<unsigned long>(val);
				val=pElemSegment->Attribute("number");
				if (val!=0) segment.number=boost::lexical_cast<long>(val);

				file.segments.push_back(segment);
			}
		}

		return nzb;
	}

	nzb loadnzb_from_file(const std::string& location, const std::filesystem::path& nzbfile)
	{
#ifdef _MSC_VER
		FILE* file=0;
		if (0!=_wfopen_s(&file,nzbfile.c_str(), L"rb"))
		{
			std::ostringstream oss;
			oss<<"Could not find file: "<<nzbfile<<std::endl;
			throw std::invalid_argument(oss.str());
		}
#else
		FILE* file = fopen(nzbfile.string().c_str(), "rb");
		if (file==0)
		{
			std::ostringstream oss;
			oss<<"Could not find file: "<<nzbfile<<std::endl;
			throw std::invalid_argument(oss.str());
		}
#endif
		tinyxml2::XMLDocument doc;
		auto xmlerr = doc.LoadFile(file);
		if (xmlerr != tinyxml2::XMLError::XML_SUCCESS)
		{
			std::ostringstream oss;
			oss<<"Could not load nzb file: "<<nzbfile<<", error is: "<<doc.ErrorStr()<<std::endl;
			throw std::invalid_argument(oss.str());
		}
		return loadnzb(location,doc);
	}

	static std::string write_data_string;

	static size_t write_data(void *contents, size_t size, size_t nmemb, void *stream)
	{
		size_t realsize = size * nmemb;
		write_data_string.append((char*)contents, realsize);
		return realsize;
	}

	nzb loadnzb_from_uri(const std::string& location, const bool usessl, const std::string& host, const std::string& port, const std::string& path)
	{
		std::string strdoc;
		
		if (usessl)
			strdoc = HTTPDownloader::LoadHTTPS(host, port, path);
		else
			strdoc = HTTPDownloader::LoadHTTP(host, port, path);

		tinyxml2::XMLDocument doc;
		auto xmlerr = doc.Parse(strdoc.c_str());
		if (xmlerr != tinyxml2::XMLError::XML_SUCCESS)
		{
			std::ostringstream oss;
			oss<<"Could not parse file at : "<<location<<", error is: "<<doc.ErrorStr()<<std::endl;
			throw std::invalid_argument(oss.str());
		}

		return loadnzb(location,doc);
	}
	nzb loadnzb(const std::string& location)
	{
		boost::match_results<std::string::const_iterator> what;
		
		if (boost::regex_search(location, what, expr_http))
		{
			bool usessl = what[1].first<what[1].second;			
			std::string host(what[2].first,what[2].second);
			std::string port(what[3].first,what[3].second);
			std::string path(what[4].first,what[4].second);
			
			if (port.length()==0) port="80";
			
			return loadnzb_from_uri(location, usessl, host, port, path);
		}
		else
		{
			std::filesystem::path nzbfile(location);
			return loadnzb_from_file(location,nzbfile);
		}
	}
}



