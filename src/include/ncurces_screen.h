#ifndef NCURCES_SCREEN_H
#define NCURCES_SCREEN_H

#include <tuple>
#include <map>

#include <viewer.h>
#include <ncurses.h>

namespace view {

class NcusrcesScreen : public Screen
{
	struct Geometry {
		int x, y, w, h;
	};

public:
	NcusrcesScreen(bool show_right_answer);
	virtual ~NcusrcesScreen();

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
