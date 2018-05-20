#ifndef PARSER_H
#define PARSER_H

#include <map>
#include <list>
#include <vector>
#include <boost/filesystem/path.hpp>

#include "problem.h"

class Parser
{
public:
	static std::vector<Problem> load(
			const boost::filesystem::path &path,
			std::map<std::string, std::vector<std::string> > params);

	static void save_statistic(
			const std::vector<Problem> &problems,
			const boost::filesystem::path &path);

	static std::map<int, int> check_answer(
			const std::list<std::string> &user_answer,
			const std::list<std::string> &right_answer);
};

#endif // PARSER_H
