#ifndef VIEWER_H
#define VIEWER_H

#include <list>
#include <map>
#include <iostream>
#include <string>

#include "problem.h"
#include "analyzer.h"

namespace view {

struct Statistic {
	int left_problems;
	int solved_problems;
	int errors;

	int problem_repeat_times;
	int problem_errors;
	int problem_total_errors;
};

enum class LANGUAGE {
	UNKNOWN,
	EN,
	RU
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

	virtual void set_language(LANGUAGE layout) = 0;
	virtual void update_statistic(const Statistic& statistic) = 0;
	virtual void show_problem(const Problem&) = 0;
	virtual void show_result(const analysis::Verification&) = 0;
	virtual void show_solution() = 0;
	virtual void show_message(const std::string& s) = 0;
};

} // namespace view

#endif // VIEWER_H
