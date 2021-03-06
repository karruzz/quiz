/*
 * parser.h
 *
 *  Created on: May 19, 2018
 *  Copyright © 2018-2081 Ilja Karasev <ilja.karasev@gmail.com>.
 *  All rights reserved.
 *     License: GNU GPL 3
 */

#ifndef PARSER_H
#define PARSER_H

#include <map>
#include <list>
#include <vector>
#include <experimental/filesystem>

#include "problem.h"

class Parser
{
public:
	static std::vector<Problem> load(
			const std::experimental::filesystem::path& path,
			const std::map<std::string, std::vector<std::string> >& params);

	static void save_statistic(
			const std::vector<Problem>& problems,
			const std::experimental::filesystem::path& path);
};

#endif // PARSER_H
