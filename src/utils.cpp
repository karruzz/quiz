/*
 * utils.cpp
 *
 *  Created on: Dec 24, 2018
 *  Copyright © 2018-2081 Ilja Karasev <ilja.karasev@gmail.com>.
 *  All rights reserved.
 *     License: GNU GPL 3
 */

#include <algorithm>
#include <stdexcept>
#include <map>

#include <cwctype>

#include <utils.h>

namespace utils {

void trim_spaces(std::string &s)
{
	auto is_space_not_tab1 = [](int c) -> int {
		return static_cast<int>(std::isspace(c) && c != '\t');
	};

	// left trim
	s.erase(s.begin(), std::find_if(s.begin(), s.end(),
		std::not1(std::ptr_fun<int, int>(is_space_not_tab1))));

	// right trim
	s.erase(std::find_if(s.rbegin(), s.rend(),
		std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

void remove_duplicate_spaces(std::string& src)
{
	static auto adjacent_spaces = [](char lhs, char rhs) { return (rhs == ' ') && (lhs == ' '); };
	src.erase(std::unique(src.begin(), src.end(), adjacent_spaces), src.end());
}

void to_lower(std::u16string& src)
{
	std::transform(src.begin(), src.end(), src.begin(), ::towlower);
}

std::u16string to_utf16(const std::string& in)
{
	std::u16string out;
	const uint8_t* s = reinterpret_cast<const uint8_t*>(in.data());
	while (*s) {
		if (*s < 0x80) {
			out.push_back(*s);
			s += 1;
		} else if ((*s >> 5) == 0x06) {
			out.push_back(((*s << 6) & 0x7FF) + (*(s+1) & 0x3F)); // integral promotion
			s += 2;
		} else if ((*s >> 4) == 0x0E) {
			out.push_back(((*s << 12) & 0xFFFF) + ((*(s+1) << 6) & 0xFFF) + ((*(s+2)) & 0x3F));
			s += 3;
		} else
			throw std::runtime_error("invalid utf8 symbol");
	}

	return out;
}

std::string to_utf8(const std::u16string& in)
{
	std::string out;
	for (uint16_t wc: in) {
		if (wc < 0x80)         // one octet
			out.push_back(static_cast<uint8_t>(wc));
		else if (wc < 0x800) { // two octets
			out.push_back(static_cast<uint8_t>((wc >> 6)          | 0xC0));
			out.push_back(static_cast<uint8_t>((wc & 0x3F)        | 0x80));
		} else {               // three octets
			out.push_back(static_cast<uint8_t>((wc >> 12)         | 0xE0));
			out.push_back(static_cast<uint8_t>(((wc >> 6) & 0x3F) | 0x80));
			out.push_back(static_cast<uint8_t>((wc & 0x3F)        | 0x80));
		}
	}
	return out;
}

Language what_language(const std::u16string& s) {
	std::map<Language, int> langs ;
	for (char16_t c: s) {
		if (c >= 0x0410 && c < 0x044F) {
			langs[Language::RU]++;
		} else if (c >= 0x0041 && c < 0x007a) {
			langs[Language::EN]++;
		} else {
			langs[Language::UNKNOWN]++;
		}
	}

	int max = 0;
	Language result = Language::UNKNOWN;
	for (auto p: langs) {
		if (p.second > max) {
			max = p.second;
			result = p.first;
		}
	}

	return result;
}

std::vector<std::string> split(const std::string& line, const std::string& delimeter)
{
	using S = std::string;
	auto gettoken = [](const S& s, size_t& begin, S& token, const S& delimeter) -> bool
	{
		size_t end = s.size();
		if (begin >= end) return false;

		size_t pos = s.find(delimeter, begin);
		if (pos != S::npos)
			end = pos;

		token = s.substr(begin, end - begin);
		begin = end + delimeter.size();
		return true;
	};

	size_t pos = 0;
	std::string token;
	std::vector<std::string> tokens;
	while(gettoken(line, pos, token, delimeter))
		tokens.push_back(token);

	return tokens;
}


} // namespace utils
