/*
 * window.cpp
 *
 *  Created on: May 19, 2018
 *  Copyright © 2018-2081 Ilja Karasev <ilja.karasev@gmail.com>.
 *  All rights reserved.
 *     License: GNU GPL 3
 */

#include <window.h>
#include <utils.h>
#include <analyzer.h>
#include <cstring>

namespace view {

namespace ncurses {

#define SERVICE_COLOR YELLOW

void Window::create()
{
	window = newwin(geometry.h, geometry.w, geometry.y, geometry.x);
	wattron(window, COLOR_PAIR(WHITE));
	wbkgd(window, COLOR_PAIR(WHITE));
}

void Window::remove()
{
	if (!window) return;
	delwin(window);
	window = NULL;
}

void Window::clear()
{
	wclear(window);
	if (geometry.h > 1) {
		wmove(window, geometry.h - 1, 0);
		whline(window, '-', geometry.w);
	}
	wmove(window, 0, 0);
}

void Window::waddstr_colored(const std::string &s, int color_scheme, bool bold)
{
	if (bold) wattron(window, A_BOLD);
	wattron(window, COLOR_PAIR(color_scheme));
	waddstr(window, s.c_str());
	wattroff(window, COLOR_PAIR(color_scheme));
	if (bold) wattroff(window, A_BOLD);
}

void Window::refresh()
{
	wrefresh(window);
}

void Window::resize(const Geometry g)
{
	remove();
	geometry = g;
	create();
	refresh();
}

void StatisticWindow::refresh()
{
	clear();

	wmove(window, 0, 1);
	waddstr_colored("Left:[" + std::to_string(statistic.left_problems) + "];", GREEN);
	waddstr_colored(" Solved:[" + std::to_string(statistic.solved_problems) + "];", CYAN);
	waddstr_colored(" Errors:[" + std::to_string(statistic.errors) + "]", RED);

	const std::string &problem = "PROBLEM:";
	const std::string &repeat = " Repeat:[" + std::to_string(statistic.problem_repeat_times) + "];";
	const std::string &errors = " Errors:[" + std::to_string(statistic.problem_errors) + "];";
	const std::string &total_errors = " Total errors:[" + std::to_string(statistic.problem_total_errors) + "]";
	wmove(window, 0, geometry.w - (problem + repeat + errors + total_errors).size() - 1);

	waddstr(window, problem.c_str());
	waddstr_colored(repeat, YELLOW);
	waddstr_colored(errors, BLUE);
	waddstr_colored(total_errors, MAGENTA);

	wrefresh(window);
}

void QuestionWindow::refresh()
{
	clear();

	int y = 0;
	for (const std::string& s: question)
		mvwaddstr(window, y++, 0, s.c_str());

	wrefresh(window);
}

void SolutionWindow::refresh()
{
	clear();
	if (visible) {
		int y = 0;
		for (const std::string& s: solution)
			mvwaddstr(window, y++, 0, s.c_str());
	}

	wrefresh(window);
}

void MessageWindow::refresh()
{
	clear();
	wmove(window, 0, 1);
	waddstr_colored(message, SERVICE_COLOR);

	std::string lan = lang_to_str();
	wmove(window, 0, geometry.w - lan.size() - 1);
	waddstr_colored(lan, SERVICE_COLOR);
	wrefresh(window);
}

static
std::vector<int> expand_tabs(const std::string& s, int tab_width)
{
	int pos = 0;
	std::vector<int> result = { pos };
	for (char c: s) {
		pos += (c != '\t') ? 1 : (tab_width - pos % tab_width);
		result.push_back(pos);
	}

	return result;
}

void AnswerWindow::update_window()
{
	clear();
	const std::vector<std::string>& answer = editor->get_lines();

	size_t y = 0;
	if (mode == Mode::INPUT) {
		for (const std::string& s: answer)
			mvwaddstr(window, y++, 0, s.c_str());
	} else if (mode == Mode::OUTPUT) {
		static auto mvwaddstr_colored = [&](int y, int x, const std::string& str, int color, bool bold = false) {
			wmove(window, y, x);
			return waddstr_colored(str, color, bold);
		};

		for (const std::string& line: verification.answer) {
			mvwaddstr(window, y, 0, line.c_str());
			const auto errors_map_it = verification.errors.find(y);
			if (errors_map_it != verification.errors.end()) {
				std::vector<int> screen_x = expand_tabs(line, tab_size);
				for (auto e: errors_map_it->second) {
					if (e.what == analysis::Error::ERROR_TOKEN)
						mvwaddstr_colored(y, screen_x[e.pos], utils::to_utf8(e.str), ERROR_WHITE, false);
					if (e.what == analysis::Error::ERROR_SYMBOL)
						mvwaddstr_colored(y, screen_x[e.pos], utils::to_utf8(e.str), ERROR_BLACK, true);
					if (e.what == analysis::Error::MISSED || e.what == analysis::Error::REDUNDANT)
						mvwaddstr_colored(y, screen_x[e.pos], utils::to_utf8(e.str), MISSED_BLACK, true);
				}
			}
			++y;
		}

		++y;
		if (verification.state == analysis::MARK::RIGHT) {
			mvwaddstr_colored(y, 0, "[right]", SERVICE_COLOR);
		} else {
			if ((verification.state & analysis::MARK::INVALID_LINES_NUMBER) != 0)
				mvwaddstr_colored(y++, 0, "[invalid lines amount]", SERVICE_COLOR);
			if ((verification.state & analysis::MARK::ERROR) != 0)
				mvwaddstr_colored(y++, 0, "[invalid answer]", SERVICE_COLOR);
			if ((verification.state & analysis::MARK::NOT_FULL_ANSWER) != 0)
				mvwaddstr_colored(y++, 0, "[not full answer]", SERVICE_COLOR);
			if ((verification.state & analysis::MARK::REDUNDANT_ANSWER) != 0)
				mvwaddstr_colored(y++, 0, "[redundant answer]", SERVICE_COLOR);
		}

	}
}

void AnswerWindow::update_line()
{
	wmove(window, editor->get_screen_y(), 0);
	wclrtoeol(window);
	waddstr(window, editor->get_current_line().c_str());
}

void AnswerWindow::refresh()
{
	update_window();
	wmove(window, editor->get_screen_y(), editor->get_screen_x());
	wrefresh(window);
}

void AnswerWindow::key_process(int key)
{
	switch (key) {
		case KEY_BACKSPACE:
			editor->backspace() ? update_window() : update_line();
			break;
		case KEY_DC:
			editor->del() ? update_window() : update_line();
			break;
		case KEY_HOME:
			editor->home();
			break;
		case KEY_END:
			editor->end();
			break;
		case KEY_PPAGE:
			editor->page_up();
			break;
		case KEY_NPAGE:
			editor->page_down();
			break;
		case KEY_RIGHT:
			editor->right();
			break;
		case KEY_LEFT:
			editor->left();
			break;
		case KEY_UP:
			editor->up();
			break;
		case KEY_DOWN:
			editor->down();
			break;
		case '\n':
			editor->new_line();
			update_window();
			break;
		case KEY_F(6):
			editor->add_str(AudioRecord::capture());
			update_line();
			break;
		default:
			if (key > KEY_CODE_YES)
				break;
			if (key < 0x80) { // one-byte octet
				editor->add_ch(key);
			} else {          // two-bytes octet
				int ch_low = wgetch(window);
				editor->add_ch(key, ch_low);
			}
			update_line();
	}

/* to log
#ifdef DEBUG
			std::stringstream ss;
			ss << keyname(ch) << "; x: " << editor.get_screen_x() << "; y: " << editor.get_screen_y();
			window_message->update(ss.str());
#endif
*/
	wmove(window, editor->get_screen_y(), editor->get_screen_x());
	wrefresh(window);
	update_cursor({ editor->get_screen_x(), editor->get_screen_y() });
}

void AnswerWindow::prepare() {
	mode = Mode::INPUT;
	editor.reset(new Editor(tab_size));
	refresh();
}

void AnswerWindow::show_analysed(const analysis::Verification& v) {
	mode = Mode::OUTPUT;
	verification = v;
	refresh();
}

std::list<std::string> AnswerWindow::get_lines()
{
	return editor->get_lines_list();
}

} // namespace ncurses

} // namespace view
