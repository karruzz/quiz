#ifndef NCURCES_SCREEN_H
#define NCURCES_SCREEN_H

#include <tuple>
#include <map>

#include <viewer.h>
#include <ncurses.h>

namespace view {

class NcursesScreen : public Screen
{
	enum color {
		RED = 1,
		GREEN = 2,
		YELLOW = 3,
		BLUE = 4,
		MAGENTA = 5,
		CYAN = 6,
		RED_BKGR = 7
	};

	struct Geometry {
		int x, y, w, h;
	};

public:
	NcursesScreen(bool show_right_answer);
	virtual ~NcursesScreen();

	virtual void update_statistic(const Statistic &s);
	virtual void show_question(const std::list<std::string> &question);
	virtual std::tuple<Screen::INPUT_STATE, std::list<std::string>> get_answer();
	virtual void show_result(
		Screen::CHECK_STATE state,
		const std::list<std::string> &right_answer,
		const std::map<int, int> &error);

	virtual void state_print(const std::string &s);

private:
	bool show_right_answer;

	WINDOW *win_statistic, *win_question, *win_answer, *win_result, *win_state;
	std::map<WINDOW *, Geometry> wins;

	void clear_win(WINDOW *w);
	WINDOW *create_win(int h, int w, int y, int x);
};

} // namespace view

#endif // NCURCES_SCREEN_H
