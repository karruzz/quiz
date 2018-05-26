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
		GREEN,
		YELLOW,
		BLUE,
		MAGENTA,
		CYAN,
		RED_BKGR,
		BKGR
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

private:
	bool show_right_answer;

	WINDOW *win_statistic, *win_question, *win_answer, *win_result, *win_state;
	std::map<WINDOW *, Geometry> wins;

	WINDOW *create_win(int h, int w, int y, int x);
	void clear_win(WINDOW *w);
	void state_print(const std::string &s);
	void deinit_all();
};

} // namespace view

#endif // NCURCES_SCREEN_H
