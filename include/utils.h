#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

void trim_spaces(std::string& s);
void remove_duplicate_spaces(std::string& s);
size_t utf8_length(const std::string& s);
std::vector<std::string> utf8_split(const std::string& s);

#endif // UTILS_H
