/*
 * utils.h
 *
 *  Created on: Dec 24, 2018
 *  Copyright © 2018-2081 Ilja Karasev <ilja.karasev@gmail.com>.
 *  All rights reserved.
 *     License: GNU GPL 3
 */

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
