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

#include <filesystem>
#include <list>
#include <map>
#include <memory>
#include <vector>

#include <options.h>
#include <problem.h>

class Parser
{
public:
	static std::vector<std::shared_ptr<Problem>> load(const Options& options);

	static void save_statistic(
		const std::vector<std::shared_ptr<Problem>>& problems,
		const std::filesystem::path& path);
};

#endif // PARSER_H
