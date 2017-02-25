/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef TEXT_TOOL_HPP
#define TEXT_TOOL_HPP

#include <string>

namespace ByteFountain
{
	namespace text_tool
	{
		extern wchar_t cp437[];
		extern wchar_t cp1252[];
		
		bool try_cp_to_unicode(const wchar_t* cp, const std::string& cptext, std::wstring& res);
#ifndef RPI
		std::wstring cp_to_unicode(const wchar_t* cp, const std::string& cptext);
		bool try_utf8_to_unicode(const std::string& utf8str, std::wstring& res);
		std::wstring utf8_to_unicode(const char* str);
#endif
		std::string unicode_to_utf8(const std::wstring& wstr);
		std::wstring utf8_to_unicode(const std::string& str);
		std::string cp_to_utf8(const wchar_t* cp, const std::string& cpstr);
		std::string strip_utf8_to_ascii(const std::string& utf8str, const unsigned char pad_char);
		std::string strip_cp_to_ascii(const std::string& cpstr, const unsigned char pad_char);
		
#ifndef _MSC_VER		
		void init();
		void shutdown();
		std::wstring decode_content_text(const std::string& str);
		std::wstring decode_header_text(const std::string& str);
		std::string u8decode_header_text(const std::string& str);
		
		bool is_ascii_text(const std::string& str);
#endif
	}
}

#endif
