#include <ncurces_screen.h>
#include <ncurses.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstring>

namespace view {

static
int waddstr_colored(WINDOW *w, const std::string &s, int color_scheme)
{
	wattron(w, COLOR_PAIR(color_scheme));
	int result = waddstr(w, s.c_str());
	wattroff(w, COLOR_PAIR(color_scheme));
	return result;
}

WINDOW *NcursesScreen::create_win(int h, int w, int y, int x)
{
	WINDOW *win = newwin(h, w, y, x);
	clear_win(win);
	wins[win] = { x, y, w, h };
	return win;
}

void NcursesScreen::clear_win(WINDOW *win)
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

void NcursesScreen::state_print(const std::string &s)
{
	clear_win(win_state);
	wmove(win_state, 0, 0);
	waddstr_colored(win_state, s.c_str(), MAGENTA);
	wrefresh(win_state);
}

#define COLOR_PAIR_INIT(value) init_pair(color::value, COLOR_##value, COLOR_BLACK)

NcursesScreen::NcursesScreen(bool show_right_answer) :
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

	COLOR_PAIR_INIT(RED);
	COLOR_PAIR_INIT(GREEN);
	COLOR_PAIR_INIT(YELLOW);
	COLOR_PAIR_INIT(BLUE);
	COLOR_PAIR_INIT(MAGENTA);
	COLOR_PAIR_INIT(CYAN);
	init_pair(color::RED_BKGR, COLOR_WHITE, COLOR_RED);

	state_print(" F2 - check answer       F3 - skip question       F10 - exit");
}

NcursesScreen::~NcursesScreen()
{
	delwin(win_statistic);
	delwin(win_question);
	delwin(win_answer);
	delwin(win_result);
	delwin(win_state);
	endwin();
}

void NcursesScreen::update_statistic(const Statistic &s)
{
	WINDOW *w = win_statistic;
	clear_win(w);

	wmove(w, 0, 1);
	waddstr_colored(w, "Total:[" + std::to_string(s.total_problems) + "];", GREEN);
	waddstr_colored(w, " Left:[" + std::to_string(s.left_problems) + "];", CYAN);
	waddstr_colored(w, " Errors:[" + std::to_string(s.errors) + "]", RED);

	wmove(w, 0, 43);
	waddstr(w, "PROBLEM:");
	waddstr_colored(w, " Repeat:[" + std::to_string(s.problem_repeat_times) + "];", YELLOW);
	waddstr_colored(w, " Errors:[" + std::to_string(s.problem_errors) + "];", BLUE);
	waddstr_colored(w, " Total errors:[" + std::to_string(s.problem_total_errors) + "]", MAGENTA);

	wrefresh(w);
}

void NcursesScreen::show_question(const std::list<std::string> &question)
{
	int y = 0;
	clear_win(win_question);
	clear_win(win_result);
	for (const std::string s: question)
		mvwaddstr(win_question, y++, 0, s.c_str());

	wrefresh(win_question);
	wrefresh(win_result);
}

class Navigator
{
	std::list<std::string> lines;
	std::list<std::string>::iterator line;

	// position every symbol on screen, need to handle tabs
	std::list<std::vector<size_t>> positions;
	std::list<std::vector<size_t>>::iterator position;

	size_t x, y, lines_total;
	int tab_size;

	void put_cursor_inside_line()
	{
		size_t line_end = line->size();
		if (x > line_end) x = line_end;
	}

	void update_positions(int index)
	{
		std::vector<size_t> &pos = *position;
		std::string str = *line;

		if (pos.size() < str.size()) pos.resize(str.size());

		size_t x_prev = (index > 0) ? pos[index - 1] : 0;
		for (size_t i = index; i < str.size(); ++i)
			if (str[i] != '\t') {
				pos[i] = ++x_prev;
			} else {
				size_t next = x_prev + tab_size;
				x_prev = next - next % tab_size;
				pos[i] = x_prev;
			}

		if (pos.size() > str.size()) pos.resize(str.size());
	}

public:
	Navigator(int tab_size) :
		x(0), y(0), lines_total(1), tab_size(tab_size)
	{
		lines = std::list<std::string>(lines_total);
		line = lines.begin();
		positions = std::list<std::vector<size_t>>(lines_total);
		position = positions.begin();
	}

	void backspace() {
		if (x != 0) {
			line->erase(--x, 1);
		} else {
			if (line == lines.begin()) return;
			std::string subst = *line;
			line = std::prev(lines.erase(line));
			position = std::prev(positions.erase(position));

			--y;
			x = line->size();
			line->append(subst);
			--lines_total;
		}

		update_positions(x);
	}

	void del() {
		if (x != line->size()) {
			line->erase(x, 1);
		} else {
			if (line == std::prev(lines.end())) return;
			auto next = std::next(line);
			std::string subst = *next;
			lines.erase(next);
			positions.erase(std::next(position));
			line->append(subst);
			--lines_total;
		}

		update_positions(x);
	}

	void home() { x = 0; }

	void end() { x = line->size(); }

	void pg_up() {
		line = lines.begin();
		position = positions.begin();
		y = 0;
		put_cursor_inside_line();
	}

	void pg_down() {
		line = std::prev(lines.end());
		position = std::prev(positions.end());
		y = lines_total - 1;
		put_cursor_inside_line();
	}

	void right() { if (x < line->size()) ++x; }

	void left() { if (x > 0) --x; }

	void up() {
		if (line == lines.begin()) return;

		--y;
		--line;
		--position;
		put_cursor_inside_line();
	}

	void down() {
		if (line == std::prev(lines.end())) return;

		++y;
		++line;
		++position;
		put_cursor_inside_line();
	}

	void new_line() {
		std::string subst = line->substr(x, line->size() - x);
		line->erase(x, line->size() - x);
		update_positions(0);

		++line;
		line = lines.insert(line, subst.c_str());
		++position;
		position = positions.insert(position, std::vector<size_t>());
		update_positions(0);

		++lines_total;
		++y;
		x = 0;
	}

	void add_ch(char ch) {
		line->insert(x, 1, ch);
		update_positions(x);
		++x;
	}

	const std::list<std::string> &get_lines() { return lines; }
	const std::string &get_current_line() { return *line; }
	size_t get_x() { return x > 0 ? (*position)[x - 1] : 0; }
	size_t get_y() { return y; }
};

std::tuple<Screen::INPUT_STATE, std::list<std::string>> NcursesScreen::get_answer()
{
	Navigator navigator(8);
	WINDOW *w = win_answer;

	auto update_screen = [&] () {
		int y = 0;
		clear_win(w);
		for (const std::string s: navigator.get_lines())
			mvwaddstr(w, y++, 0, s.c_str());
	};

	clear_win(w);
	wmove(w, 0, 0);
	wrefresh(w);

	for (;;) {
		int ch = wgetch(w);

		if (ch == ERR) {
			throw std::runtime_error("wgetch error");
		} else if (ch == KEY_F(2)) { // finish answer
			return std::make_tuple(Screen::INPUT_STATE::ENTERED, navigator.get_lines());
		} else if (ch == KEY_F(3)) { // skip question
			return std::make_tuple(Screen::INPUT_STATE::SKIPPED, std::list<std::string>());
		} else if (ch == KEY_F(10)) { // exit
			return std::make_tuple(Screen::INPUT_STATE::EXIT, std::list<std::string>());
		} else {
			switch (ch) {
				case KEY_BACKSPACE:
					navigator.backspace();
					update_screen();
					break;
				case KEY_DC:
					navigator.del();
					update_screen();
					break;
				case KEY_HOME:
					navigator.home();
					break;
				case KEY_END:
					navigator.end();
					break;
				case KEY_PPAGE:
					navigator.pg_up();
					break;
				case KEY_NPAGE:
					navigator.pg_down();
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
					update_screen();
					break;
				default:
					navigator.add_ch(ch);
					wmove(w, navigator.get_y(), 0);
					wclrtoeol(w);
					waddstr(w, navigator.get_current_line().c_str());
			}

#ifdef DEBUG
			state_print(keyname(ch));
#endif
			wmove(w, navigator.get_y(), navigator.get_x());
			wrefresh(w);
		}
	}
}

void NcursesScreen::show_result(
		Screen::CHECK_STATE state,
		const std::list<std::string> &answer,
		const std::map<int, int> &errors)
{
	WINDOW *win = win_result;
	clear_win(win);

	auto mvwaddstr_colored = [&](int y, int x, const char *str, int color_scheme) {
		wmove(win, y, x);
		return waddstr_colored(win, str, color_scheme);
	};

	if (state == Screen::CHECK_STATE::RIGHT) {
		mvwaddstr_colored(0, 0, "[right]", MAGENTA);
	} else if (state == Screen::CHECK_STATE::LINES_NUMBER_ERROR) {
		int line = 0;
		if (show_right_answer)
			for (const std::string s: answer)
				mvwaddstr(win, line++, 0, s.c_str());

		mvwaddstr_colored(line, 0, "[invalid lines amount]", MAGENTA);
	} else if (state == Screen::CHECK_STATE::INVALID) {
		if (show_right_answer) {
			int line = 0;
			std::list<std::string>::const_iterator it = answer.begin();
			for (; it != answer.end(); ++it, ++line) {
				const auto error_it = errors.find(line);
				if (error_it != errors.end()) {
					int error_x = error_it->second;
					mvwaddstr(win, line, 0, it->substr(0, error_x).c_str());
					mvwaddstr_colored(line, error_x, it->substr(error_x).c_str(), RED_BKGR);
					continue;
				}
				mvwaddstr(win, line, 0, it->c_str());
			}
		} else
			mvwaddstr_colored(0, 0, "[invalid answer]", MAGENTA);
	} else if (state == Screen::CHECK_STATE::SKIPPED) {
		mvwaddstr_colored(0, 0, "[skipped]", MAGENTA);
	} else if (state == Screen::CHECK_STATE::ALL_SOLVED) {
		mvwaddstr_colored(0, 0, "[all problems are solved]", MAGENTA);
	}

	wrefresh(win);
	wgetch(win);
}

} // namespace view

