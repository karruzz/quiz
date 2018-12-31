#include <algorithm>
#include <stdexcept>

#include <cwctype>

#include <utils.h>

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
