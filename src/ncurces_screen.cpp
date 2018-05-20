#include <ncurces_screen.h>
#include <ncurses.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace view {

static
int mvwaddstr_colored(WINDOW *win, int y, int x, const char *str, int color_scheme) {
	wattron(win, COLOR_PAIR(color_scheme));
	int result = mvwaddstr(win, y, x, str);
	wattroff(win, COLOR_PAIR(color_scheme));
	return result;
}

WINDOW *NcusrcesScreen::create_win(int h, int w, int y, int x)
{
	WINDOW *win = newwin(h, w, y, x);
	clear_win(win);
	wins[win] = { x, y, w, h };
	return win;
}

void NcusrcesScreen::clear_win(WINDOW *win)
{
	const Geometry &g = wins[win];
	wclear(win);
	if (g.h > 1) {
		wmove(win, g.h - 1, 0);
		whline(win, '-', g.w + 1);
		wmove(win, 0, 0);
	}
	wrefresh(win);
}

void NcusrcesScreen::state_print(const std::string &s)
{
	clear_win(win_state);
	wmove(win_state, 0, 0);
	waddstr(win_state, s.c_str());
	wrefresh(win_state);
}

NcusrcesScreen::NcusrcesScreen(bool show_right_answer) :
	show_right_answer(show_right_answer)
{
	initscr();
	start_color();
	cbreak();
	noecho();

	const int stat_h = 2, state_h = 1;
	int ques_h = (LINES - stat_h - state_h) * 2 / 5;
	int answ_h = (LINES - stat_h - ques_h - state_h) / 2;
	int resl_h =  LINES - stat_h - ques_h - state_h - answ_h;

	win_statistic = create_win(stat_h,  COLS - 1, 0, 0);
	win_question  = create_win(ques_h,  COLS - 1, stat_h, 0);
	win_answer    = create_win(answ_h,  COLS - 1, stat_h + ques_h, 0);
	win_result    = create_win(resl_h,  COLS - 1, stat_h + ques_h + answ_h, 0);
	win_state     = create_win(state_h, COLS - 1, stat_h + ques_h + answ_h + resl_h, 0);

	keypad(win_answer, TRUE);
	init_pair(1, COLOR_WHITE, COLOR_RED);
	init_pair(2, COLOR_MAGENTA, COLOR_BLACK);

	state_print(" F2 - check answer       F3 - skip question       ESC - exit");
}

NcusrcesScreen::~NcusrcesScreen()
{
	delwin(win_statistic);
	delwin(win_question);
	delwin(win_answer);
	delwin(win_result);
	delwin(win_state);
	endwin();
}

void NcusrcesScreen::update_statistic(const Statistic &s)
{
	auto cout_stat = [&](const std::string &s) { return waddstr(win_statistic, s.c_str()); };

	clear_win(win_statistic);

	wmove(win_statistic, 0, 1);
	cout_stat("ALL: Total:[" + std::to_string(s.test_total_problems) + "];");
	cout_stat(" Left:[" + std::to_string(s.test_left_problems) + "];");
	cout_stat(" Errors:[" + std::to_string(s.test_errors) + "]");

	wmove(win_statistic, 0, 43);
	cout_stat("PROBLEM: Repeat:[" + std::to_string(s.problem_repeat_times) + "];");
	cout_stat(" Errors:[" + std::to_string(s.problem_errors) + "];");
	cout_stat(" Total errors:[" + std::to_string(s.problem_total_errors) + "]");

	wrefresh(win_statistic);
}

void NcusrcesScreen::show_question(const std::list<std::string> &question)
{
	int y = 0;
	clear_win(win_question);
	clear_win(win_result);
	for (const std::string s: question)
		mvwaddstr(win_question, y++, 0, s.c_str());

	wrefresh(win_question);
	wrefresh(win_result);
}

struct Navigator {
	Navigator() :
		pos_x(0), pos_y(0), lines_total(1)
	{
		lines = std::list<std::string>(lines_total);
		line = lines.begin();
	}

	bool backspace() {
		if (pos_x == 0) return false;

		line->erase(--pos_x, 1);
		return true;
	}

	bool del() {
		if (line->size() == 0 || pos_x == line->size()) return false;

		line->erase(pos_x, 1);
		return true;
	}

	void right() {
		if (pos_x < line->size()) ++pos_x;
	}

	void left() {
		if (pos_x > 0) --pos_x;
	}

	void up() {
		if (line == lines.begin()) return;

		--pos_y;
		--line;
		size_t line_end = line->size();
		if (pos_x > line_end) pos_x = line_end;
	}

	void down() {
		if (line == --lines.end()) return;

		++pos_y;
		++line;
		size_t line_end = line->size();
		if (pos_x > line_end) pos_x = line_end;
	}

	std::string new_line() {
		std::string subst = line->substr(pos_x, line->size() - pos_x);
		line->erase(pos_x, line->size() - pos_x);
		line = lines.insert(++line, subst.c_str());
		++lines_total;

		pos_x = 0;
		++pos_y;

		return subst;
	}

	void add_ch(char ch) {
		line->insert(pos_x++, 1, ch);
	}

	const std::list<std::string> &get_lines() { return lines; }
	size_t get_x() { return pos_x; }
	size_t get_y() { return pos_y; }

private:
	std::list<std::string> lines;
	std::list<std::string>::iterator line;
	size_t pos_x = 0, pos_y = 0, lines_total = 1;
};

std::tuple<Screen::INPUT_STATE, std::list<std::string>> NcusrcesScreen::get_answer()
{
	Navigator navigator;

	clear_win(win_answer);
	wmove(win_answer, 0, 0);
	wrefresh(win_answer);

	size_t y = 0;
	for (;;) {
		int ch = wgetch(win_answer);

		if (ch == ERR) {
			throw std::runtime_error("wgetch error");
		} else if (ch == KEY_F(2)) { // finish answer
			return std::make_tuple(Screen::INPUT_STATE::ENTERED, navigator.get_lines());
		} else if (ch == KEY_F(3)) { // skip question
			return std::make_tuple(Screen::INPUT_STATE::SKIPPED, navigator.get_lines());
		} else if (ch == 27) { // key esc, exit
			return std::make_tuple(Screen::INPUT_STATE::EXIT, navigator.get_lines());
		} else {
			switch (ch) {
				case KEY_BACKSPACE:
					if (navigator.backspace()) {
						wmove(win_answer, navigator.get_y(), navigator.get_x());
						wdelch(win_answer);
					}
					break;
				case KEY_DC:
					if (navigator.del())
						wdelch(win_answer);
					break;
				case KEY_RIGHT:
					navigator.right();
					break;
				case KEY_LEFT:
					navigator.left();
					break;
				case KEY_UP:
					navigator.up();
					break;
				case KEY_DOWN:
					navigator.down();
					break;
				case '\n':
					navigator.new_line();

					y = 0;
					clear_win(win_answer);
					for (const std::string s: navigator.get_lines())
						mvwaddstr(win_answer, y++, 0, s.c_str());

					break;
				default:
					navigator.add_ch(ch);
					winsch(win_answer, ch);
			}

#ifdef DEBUG
			state_print(keyname(ch));
#endif
			wmove(win_answer, navigator.get_y(), navigator.get_x());
			wrefresh(win_answer);
		}
	}
}

void NcusrcesScreen::show_result(
		Screen::CHECK_STATE state,
		const std::list<std::string> &answer,
		const std::map<int, int> &errors)
{
	WINDOW *win = win_result;
	clear_win(win);

	if (state == Screen::CHECK_STATE::RIGHT) {
		mvwaddstr_colored(win, 0, 0, "[right]", 2);
	} else if (state == Screen::CHECK_STATE::LINES_NUMBER_ERROR) {
		int line = 0;
		if (show_right_answer)
			for (const std::string s: answer)
				mvwaddstr(win, line++, 0, s.c_str());

		mvwaddstr_colored(win, line, 0, "[invalid lines number]", 2);
	} else if (state == Screen::CHECK_STATE::INVALID) {
		if (show_right_answer) {
			int line = 0;
			std::list<std::string>::const_iterator it = answer.begin();
			for (; it != answer.end(); ++it, ++line) {
				const auto error_it = errors.find(line);
				if (error_it != errors.end()) {
					int error_position = error_it->second;
					mvwaddstr(win, line, 0, it->substr(0, error_position).c_str());
					mvwaddstr_colored(win, line, error_position, it->substr(error_position).c_str(), 1);
					continue;
				}
				mvwaddstr(win, line, 0, it->c_str());
			}
		} else
			mvwaddstr_colored(win, 0, 0, "[invalid answer]", 2);
	} else if (state == Screen::CHECK_STATE::SKIPPED) {
		mvwaddstr_colored(win, 0, 0, "[skipped]", 2);
	} else if (state == Screen::CHECK_STATE::ALL_SOLVED) {
		mvwaddstr_colored(win, 0, 0, "[all problems are solved]", 2);
	}

	wrefresh(win);
	wgetch(win);
}

} // namespace view

