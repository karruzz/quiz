#include <algorithm>
#include <stdexcept>

#include <utils.h>

namespace {

int is_space_not_tab(int c)
{
	return static_cast<int>(std::isspace(c) && c != '\t');
}

void ltrim(std::string &s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(),
			std::not1(std::ptr_fun<int, int>(is_space_not_tab))));
}

void rtrim(std::string &s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(),
			std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

int octets_count(char c)
{
	uint8_t lead = static_cast<uint8_t>(c);
	if (lead < 0x80)         return 1;
	if ((lead >> 5) == 0x06) return 2;
	if ((lead >> 4) == 0x0E) return 3;
	if ((lead >> 3) == 0x1E) return 4;
	throw std::runtime_error("invalid utf8 symbol");
}

} // namespace

void trim_spaces(std::string &s)
{
	ltrim(s);
	rtrim(s);
}

void remove_duplicate_spaces(std::string& src)
{
	static auto adjacent_spaces = [](char lhs, char rhs) { return (rhs == ' ') && (lhs == ' '); };
	src.erase(std::unique(src.begin(), src.end(), adjacent_spaces), src.end());
}

size_t utf8_length(const std::string& s)
{
	size_t len = 0;
	for (size_t i = 0; i < s.size(); i += octets_count(s.at(i)), ++len);
	return len;
}

std::vector<std::string> utf8_split(const std::string& s)
{
	std::vector<std::string> result;
	int len = 0;
	for (size_t i = 0; i < s.size(); i += len) {
		len = octets_count(s.at(i));
		result.push_back(s.substr(i, len));
	}
	return result;
}
