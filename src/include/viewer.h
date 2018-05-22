#ifndef VIEWER_H
#define VIEWER_H

#include <list>
#include <map>
#include <iostream>
#include <string>

namespace view {

struct Statistic {
	int total_problems;
	int left_problems;
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

	enum class CHECK_STATE {
		RIGHT,
		SKIPPED,
		LINES_NUMBER_ERROR,
		INVALID,
		ALL_SOLVED
	};

	virtual ~Screen() { }

	virtual void update_statistic(const Statistic &statistic) = 0;
	virtual void show_question(const std::list<std::string> &question) = 0;
	virtual std::tuple<INPUT_STATE, std::list<std::string>> get_answer() = 0;
	virtual void show_result(
		Screen::CHECK_STATE state,
		const std::list<std::string> &right_answer,
		const std::map<int, int> &error) = 0;

	virtual void state_print(const std::string &s) = 0;
};

class Viewer {
public:
	enum color {
		RED = 31,
		GREEN = 32,
		YELLOW = 33,
		BLUE = 34,
		MAGNETTA = 35,
		CYAN = 36,
		RED_BACKGROUND = 41,
	};

	static void clear_screen();
	static void colored_line_output(const std::string& s, Viewer::color c, bool new_line = true);
};

} // namespace view

#endif // VIEWER_H
