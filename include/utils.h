#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

void trim_spaces(std::string& s);
void remove_duplicate_spaces(std::string& s);
void to_lower(std::u16string& s);

// only Basic Multilingual Plane
std::u16string to_utf16(const std::string& s);
std::string to_utf8(const std::u16string& s);

#endif // UTILS_H
