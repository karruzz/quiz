#include <ncurces_screen.h>
#include <ncursesw/ncurses.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <cstdlib>

#include <record.h>

#define TAB_SIZE 4

#define COLOR_LIGHT_WHITE 15
#define COLOR_GRAY 8
#define SERVICE_COLOR YELLOW

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
	wattron(win, COLOR_PAIR(BKGR));
	wbkgd(win, COLOR_PAIR(BKGR));
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
	waddstr_colored(win_state, s, SERVICE_COLOR);
	wrefresh(win_state);
}

NcursesScreen::NcursesScreen(bool show_right_answer) :
	show_right_answer(show_right_answer)
{
	initscr();
	set_tabsize(TAB_SIZE);
	start_color();
	cbreak();
	noecho();

	const int stat_h = 2, state_h = 1;
	int ques_h = (LINES - stat_h - state_h) * 2 / 7;
	int answ_h = (LINES - stat_h - ques_h - state_h) / 2;
	int resl_h =  LINES - stat_h - ques_h - state_h - answ_h;

	init_pair(CYAN, 14, COLOR_GRAY);
	init_pair(RED, 9, COLOR_GRAY);
	init_pair(GREEN, 10, COLOR_GRAY);
	init_pair(YELLOW, 11, COLOR_GRAY);
	init_pair(BLUE, 12, COLOR_GRAY);
	init_pair(MAGENTA, 13, COLOR_GRAY);

	init_pair(RED_BKGR, COLOR_LIGHT_WHITE, COLOR_RED);
	init_pair(BKGR, COLOR_LIGHT_WHITE, COLOR_GRAY);

	win_statistic = create_win(stat_h,  COLS - 1, 0, 0);
	win_question  = create_win(ques_h,  COLS - 1, stat_h, 0);
	win_answer    = create_win(answ_h,  COLS - 1, stat_h + ques_h, 0);
	win_result    = create_win(resl_h,  COLS - 1, stat_h + ques_h + answ_h, 0);
	win_state     = create_win(state_h, COLS - 1, stat_h + ques_h + answ_h + resl_h, 0);

	keypad(win_answer, TRUE);
	keypad(win_result, TRUE);

	state_print(" F2 - check answer       F3 - skip question       F4 - capture audio       F10 - exit");
}

void NcursesScreen::deinit_all() {
	delwin(win_statistic);
	delwin(win_question);
	delwin(win_answer);
	delwin(win_result);
	delwin(win_state);
	endwin();
}

NcursesScreen::~NcursesScreen()
{
	deinit_all();
}

void NcursesScreen::update_statistic(const Statistic &s)
{
	WINDOW *w = win_statistic;
	clear_win(w);

	wmove(w, 0, 1);
	waddstr_colored(w, "Total:[" + std::to_string(s.total_problems) + "];", GREEN);
	waddstr_colored(w, " Solved:[" + std::to_string(s.solved_problems) + "];", CYAN);
	waddstr_colored(w, " Errors:[" + std::to_string(s.errors) + "]", RED);

	const std::string &problem = "PROBLEM:";
	const std::string &repeat = " Repeat:[" + std::to_string(s.problem_repeat_times) + "];";
	const std::string &errors = " Errors:[" + std::to_string(s.problem_errors) + "];";
	const std::string &total_errors = " Total errors:[" + std::to_string(s.problem_total_errors) + "]";
	wmove(w, 0, COLS - (problem + repeat + errors + total_errors).size() - 2);

	waddstr(w, problem.c_str());
	waddstr_colored(w, repeat, YELLOW);
	waddstr_colored(w, errors, BLUE);
	waddstr_colored(w, total_errors, MAGENTA);

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
	const size_t tab_width;
	const size_t ascii_width;
	const size_t utf8_width;

	std::vector<std::string> lines;
	// position of every symbol of lines on screen, need to handle tabs and utf-8 pair symbols
	std::vector<std::vector<size_t>> screen_positions;

	size_t cursor_x, cursor_y;

	bool is_ascii(char c) { return c > 0; }
	size_t sym_width(char c) { return is_ascii(c) ? ascii_width : utf8_width; }

	void move_cursor_inside_line()
	{
		size_t line_end = lines[cursor_y].size();
		if (cursor_x > line_end) cursor_x = line_end;
	}

	void update_screen_positions(size_t x, size_t y)
	{
		if (screen_positions[y].size() != lines[y].size())
			screen_positions[y].resize(lines[y].size());

		size_t screen_x = (x > 0) ? screen_positions[y][x - 1] : 0;
		while(x < lines[y].size()) {
			if (lines[y][x] == '\t') {
				size_t next = screen_x + tab_width;
				screen_x = next - next % tab_width;
			} else
				++screen_x;

			size_t w = sym_width(lines[y][x]);
			for (size_t j = 0; j < w; ++j)
				screen_positions[y][x + j] = screen_x;

			x += w;
		}
	}

public:
	explicit Navigator(size_t tab_size)
		: tab_width(tab_size)
		, ascii_width(1)
		, utf8_width(2)
		, cursor_x(0)
		, cursor_y(0)
	{
		lines = std::vector<std::string>(1);
		screen_positions = std::vector<std::vector<size_t>>(1);
	}

	void backspace() {
		if (cursor_x != 0) {
			size_t width = sym_width(lines[cursor_y][cursor_x - 1]);
			cursor_x -=  width;
			lines[cursor_y].erase(cursor_x, width);
		} else { // begin of line
			if (cursor_y == 0) return;
			std::string subst = lines[cursor_y];
			lines.erase(lines.begin() + cursor_y);
			screen_positions.erase(screen_positions.begin() + cursor_y);
			--cursor_y;
			cursor_x = lines[cursor_y].size();
			lines[cursor_y].append(subst);
		}

		update_screen_positions(cursor_x, cursor_y);
	}

	void del() {
		if (cursor_x != lines[cursor_y].size()) {
			size_t width = sym_width(lines[cursor_y][cursor_x]);
			lines[cursor_y].erase(cursor_x, width);
		} else { // end of line
			if (cursor_y == lines.size() - 1) return;
			std::string subst = lines[cursor_y + 1];
			lines.erase(lines.begin() + cursor_y + 1);
			screen_positions.erase(screen_positions.begin()  + cursor_y + 1);
			lines[cursor_y].append(subst);
		}

		update_screen_positions(cursor_x, cursor_y);
	}

	void home() { cursor_x = 0; }

	void end() { cursor_x = lines[cursor_y].size(); }

	void page_up() {
		cursor_y = 0;
		move_cursor_inside_line();
	}

	void page_down() {
		cursor_y = lines.size() - 1;
		move_cursor_inside_line();
	}

	void right() {
		if (cursor_x >= lines[cursor_y].size()) return;
		cursor_x += sym_width(lines[cursor_y][cursor_x]);
	}

	void left() {
		if (cursor_x == 0) return;
		cursor_x -= sym_width(lines[cursor_y][cursor_x - 1]);
	}

	void up() {
		if (cursor_y == 0) return;
		--cursor_y;
		move_cursor_inside_line();
	}

	void down() {
		if (cursor_y == lines.size() - 1) return;
		++cursor_y;
		move_cursor_inside_line();
	}

	void new_line() {
		std::string &&remaining_subst = lines[cursor_y].substr(cursor_x);
		lines[cursor_y].erase(cursor_x);
		update_screen_positions(cursor_x, cursor_y);

		++cursor_y;
		lines.insert(lines.begin() + cursor_y, remaining_subst);
		screen_positions.insert(screen_positions.begin() + cursor_y, std::vector<size_t>());
		cursor_x = 0;
		update_screen_positions(cursor_x, cursor_y);
	}

	void add_ch(char ch) {
		lines[cursor_y].insert(cursor_x, 1, ch);
		update_screen_positions(cursor_x, cursor_y);
		cursor_x += ascii_width;
	}

	void add_wch(char ch_high, char ch_low) {
		lines[cursor_y].insert(cursor_x, 1, ch_low);
		lines[cursor_y].insert(cursor_x, 1, ch_high);
		update_screen_positions(cursor_x, cursor_y);
		cursor_x += utf8_width;
	}

	void add_str(const std::string &s) {
		lines[cursor_y].insert(cursor_x, s);
		update_screen_positions(cursor_x, cursor_y);
		cursor_x += s.length();
	}

	const std::vector<std::string> &get_lines() { return lines; }

	std::list<std::string> get_lines_list() {
		std::list<std::string> result;
		std::copy( lines.begin(), lines.end(), std::back_inserter( result ) );
		return result;
	}

	const std::string &get_current_line() { return lines[cursor_y]; }
	size_t get_screen_x() { return cursor_x > 0 ? screen_positions[cursor_y][cursor_x - 1] : 0; }
	size_t get_screen_y() { return cursor_y; }
};

std::tuple<Screen::INPUT_STATE, std::list<std::string>> NcursesScreen::get_answer()
{
	AudioRecord audio_record;
	Navigator navigator(TAB_SIZE);
	WINDOW *w = win_answer;

	auto update_screen = [&] () {
		int y = 0;
		clear_win(w);
		for (const std::string s: navigator.get_lines())
			mvwaddstr(w, y++, 0, s.c_str());
	};

	auto update_line = [&] () {
		wmove(w, navigator.get_screen_y(), 0);
		wclrtoeol(w);
		waddstr(w, navigator.get_current_line().c_str());
	};

	clear_win(w);
	wmove(w, 0, 0);
	wrefresh(w);

	for (;;) {
		int ch = wgetch(w);

		if (ch == ERR) {
			throw std::runtime_error("wgetch error");
		} else if (ch == KEY_F(2)) { // finish answer
			return std::make_tuple(Screen::INPUT_STATE::ENTERED, navigator.get_lines_list());
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
					navigator.page_up();
					break;
				case KEY_NPAGE:
					navigator.page_down();
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
				case KEY_F(4):
					navigator.add_str(audio_record.capture());
					update_line();
					break;
				default:
					if (ch <= 128) {
						navigator.add_ch(ch);
					} else {
						int ch_low = wgetch(w);
						navigator.add_wch(ch, ch_low);
					}
					update_line();
			}

#ifdef DEBUG
			std::stringstream ss;
			ss << keyname(ch) << "; x: " << navigator.get_screen_x() << "; y: " << navigator.get_screen_y();
			state_print(ss.str());
#endif
			wmove(w, navigator.get_screen_y(), navigator.get_screen_x());
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
		mvwaddstr_colored(0, 0, "[right]", SERVICE_COLOR);
	} else if (state == Screen::CHECK_STATE::LINES_NUMBER_ERROR) {
		int line = 0;
		if (show_right_answer)
			for (const std::string s: answer)
				mvwaddstr(win, line++, 0, s.c_str());

		mvwaddstr_colored(line, 0, "[invalid lines amount]", SERVICE_COLOR);
	} else if (state == Screen::CHECK_STATE::INVALID) {
		if (show_right_answer) {
			int line = 0;
			std::list<std::string>::const_iterator it = answer.begin();
			for (; it != answer.end(); ++it, ++line) {
				const auto error_it = errors.find(line);
				if (error_it != errors.end()) {
					int error_since_sym = error_it->second;
					mvwaddstr(win, line, 0, it->substr(0, error_since_sym).c_str());
					waddstr_colored(win, it->substr(error_since_sym), RED_BKGR);
					continue;
				}
				mvwaddstr(win, line, 0, it->c_str());
			}
		} else
			mvwaddstr_colored(0, 0, "[invalid answer]", SERVICE_COLOR);
	} else if (state == Screen::CHECK_STATE::SKIPPED) {
		mvwaddstr_colored(0, 0, "[skipped]", SERVICE_COLOR);
	} else if (state == Screen::CHECK_STATE::ALL_SOLVED) {
		mvwaddstr_colored(0, 0, "[all problems are solved]", SERVICE_COLOR);
	}

	wrefresh(win);
	if (wgetch(win) != KEY_F(10)) return;

	deinit_all();
	std::exit(EXIT_SUCCESS);
}

} // namespace view

