/*
 * viewer.h
 *
 *  Created on: May 19, 2018
 *  Copyright © 2018-2081 Ilja Karasev <ilja.karasev@gmail.com>.
 *  All rights reserved.
 *     License: GNU GPL 3
 */

#ifndef VIEWER_H
#define VIEWER_H

#include <list>
#include <map>
#include <iostream>
#include <string>

#include "problem.h"
#include "analyzer.h"
#include "utils.h"

namespace view {

enum FKEY {
	F3 = 0xF3F3, // need to prevent dependence from ncurses.h in quiz.cpp
};

struct Statistic {
	int left_problems;
	int solved_problems;
	int errors;

	int problem_repeat_times;
	int problem_errors;
	int problem_total_errors;
};

class Screen
{
public:
	enum class INPUT_STATE {
		ENTERED,
		SKIPPED,
		EXIT
	};

	virtual ~Screen() { }

	virtual std::tuple<INPUT_STATE, std::list<std::string>> get_answer() = 0;
	virtual int wait_pressed_key() = 0;

	virtual void set_language(utils::LANGUAGE layout) = 0;
	virtual void update_statistic(const Statistic& statistic) = 0;
	virtual void show_problem(const Problem&) = 0;
	virtual void show_result(const analysis::Verification&) = 0;
	virtual void show_solution() = 0;
	virtual void show_message(const std::string& s) = 0;
};

} // namespace view

#endif // VIEWER_H
