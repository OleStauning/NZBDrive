/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#include "nzb.hpp"
#include "nzb_fileloader.hpp"
#include "text_tool.hpp"
#include <tinyxml.h>
#include <boost/lexical_cast.hpp>

namespace ByteFountain
{

	nzb loadnzb(const boost::filesystem::path& nzbfile, TiXmlDocument& doc, std::string& err_msg)
	{

		TiXmlHandle hDoc(&doc);
		TiXmlElement* pElem;
		TiXmlHandle hRoot(0);

		// block: nzb
		{
			pElem=hDoc.FirstChildElement().Element();
			// should always have a valid root but handle gracefully if it does
			if (!pElem || pElem->Value()!=std::string("nzb") )
			{
				std::ostringstream oss;
				oss<<"Invalid nzb file: "<<nzbfile<<std::endl;
				err_msg=oss.str();
				return nzb();		
			}
			// save this for later
			hRoot=TiXmlHandle(pElem);
		}

		ByteFountain::nzb nzb;

		TiXmlElement* pElemFile=hRoot.FirstChild( "file" ).Element();
		if (!pElemFile)
		{
			std::ostringstream oss;
			oss<<"Invalid nzb file (no file-tag): "<<nzbfile<<std::endl;
			err_msg=oss.str();
			return nzb;
		}
		size_t nfiles=0;
		for(/*nothing*/; pElemFile; pElemFile=pElemFile->NextSiblingElement()) ++nfiles;

		pElemFile=hRoot.FirstChild( "file" ).Element();
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

			TiXmlElement* pElemGroup=TiXmlHandle(pElemFile).FirstChild( "groups" ).FirstChild( "group" ).Element();
			if (!pElemGroup)
			{
				std::ostringstream oss;
				oss<<"Invalid nzb file (no group-tag): "<<nzbfile<<std::endl;
				err_msg=oss.str();
				return nzb;
			}
			for(/*nothing*/; pElemGroup; pElemGroup=pElemGroup->NextSiblingElement())
			{
				file.groups.insert(pElemGroup->GetText());
			}

			TiXmlElement* pElemSegment=TiXmlHandle(pElemFile).FirstChild( "segments" ).FirstChild( "segment" ).Element();
			if (!pElemSegment)
			{
				std::ostringstream oss;
				oss<<"Invalid nzb file (no segment-tag): "<<nzbfile<<std::endl;
				err_msg=oss.str();
				return nzb;
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

	nzb loadnzb(const boost::filesystem::path& nzbfile, std::string& err_msg)
	{
		err_msg.clear();
#ifdef WIN32
		FILE* file=0;
		if (0!=_wfopen_s(&file,nzbfile.c_str(), L"rb"))
		{
			std::ostringstream oss;
			oss<<"Could not find file: "<<nzbfile<<std::endl;
			err_msg=oss.str();
			return nzb();		
		}
#else
		FILE* file = fopen(nzbfile.string().c_str(), "rb");
#endif
		TiXmlDocument doc;
		if (!doc.LoadFile(file))
		{
			std::ostringstream oss;
			oss<<"Could not load nzb file: "<<nzbfile<<std::endl;
			err_msg=oss.str();
			return nzb();		
		}
		return loadnzb(nzbfile,doc,err_msg);
	}

}
