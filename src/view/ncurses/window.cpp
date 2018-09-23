#include <window.h>

namespace view {

namespace ncurses {

#define SERVICE_COLOR YELLOW

Window::Window(const Geometry &g)
{
	geometry = g;
	create();
	clear();
}

Window::~Window()
{
	remove();
}

void Window::create()
{
	window = newwin(geometry.h, geometry.w, geometry.y, geometry.x);
	wattron(window, COLOR_PAIR(BKGR));
	wbkgd(window, COLOR_PAIR(BKGR));
}

void Window::remove()
{
	delwin(window);
}

void Window::clear()
{
	wclear(window);
	if (geometry.h > 1) {
		wmove(window, geometry.h - 1, 0);
		whline(window, '-', geometry.w);
	}
	wmove(window, 0, 0);
	wrefresh(window);
}

void Window::waddstr_colored(const std::string &s, int color_scheme)
{
	wattron(window, COLOR_PAIR(color_scheme));
	waddstr(window, s.c_str());
	wattroff(window, COLOR_PAIR(color_scheme));
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
	waddstr_colored("Total:[" + std::to_string(statistic.total_problems) + "];", GREEN);
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
	for (const std::string s: question)
		mvwaddstr(window, y++, 0, s.c_str());

	wrefresh(window);
}

void ResultWindow::refresh()
{
	clear();
	if (!enable_result) return;

	auto mvwaddstr_colored = [&](int y, int x, const char *str, int color_scheme) {
		wmove(window, y, x);
		return waddstr_colored(str, color_scheme);
	};

	if (state == Screen::CHECK_STATE::RIGHT) {
		mvwaddstr_colored(0, 0, "[right]", SERVICE_COLOR);
	} else if (state == Screen::CHECK_STATE::LINES_NUMBER_ERROR) {
		int line = 0;
		if (show_right_answer)
			for (const std::string s: answer)
				mvwaddstr(window, line++, 0, s.c_str());

		mvwaddstr_colored(line, 0, "[invalid lines amount]", SERVICE_COLOR);
	} else if (state == Screen::CHECK_STATE::INVALID) {
		if (show_right_answer) {
			int line = 0;
			std::list<std::string>::const_iterator it = answer.begin();
			for (; it != answer.end(); ++it, ++line) {
				const auto error_it = errors.find(line);
				if (error_it != errors.end()) {
					int error_since_sym = error_it->second;
					mvwaddstr(window, line, 0, it->substr(0, error_since_sym).c_str());
					waddstr_colored(it->substr(error_since_sym), RED_BKGR);
					continue;
				}
				mvwaddstr(window, line, 0, it->c_str());
			}
		} else
			mvwaddstr_colored(0, 0, "[invalid answer]", SERVICE_COLOR);
	} else if (state == Screen::CHECK_STATE::SKIPPED) {
		mvwaddstr_colored(0, 0, "[skipped]", SERVICE_COLOR);
	} else if (state == Screen::CHECK_STATE::ALL_SOLVED) {
		mvwaddstr_colored(0, 0, "[all problems are solved]", SERVICE_COLOR);
	}

	wrefresh(window);
}

void MessageWindow::refresh()
{
	clear();
	wmove(window, 0, 0);
	waddstr_colored(message, SERVICE_COLOR);
	wrefresh(window);
}

void AnswerWindow::update_screen()
{
	clear();
	int y = 0;
	for (const std::string s: editor->get_lines())
		mvwaddstr(window, y++, 0, s.c_str());
}

void AnswerWindow::update_line()
{
	wmove(window, editor->get_screen_y(), 0);
	wclrtoeol(window);
	waddstr(window, editor->get_current_line().c_str());
}

void AnswerWindow::refresh()
{
	update_screen();
	wmove(window, editor->get_screen_y(), editor->get_screen_x());
	wrefresh(window);
}

void AnswerWindow::key_pressed(int key)
{
	switch (key) {
		case KEY_BACKSPACE:
			editor->backspace();
			update_screen();
			break;
		case KEY_DC:
			editor->del();
			update_screen();
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
			update_screen();
			break;
		case KEY_F(4):
			editor->add_str(audio_record.capture());
			update_line();
			break;
		default:
			if (key <= 128) { // ascii
				editor->add_ch(key);
			} else { // utf-8
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
	editor.reset(new Editor(tab_size));
	update_cursor({ editor->get_screen_x(), editor->get_screen_y() });
	update_screen();
}

std::list<std::string> AnswerWindow::get_answer()
{
	return editor->get_lines_list();
}

} // namespace ncurses

} // namespace view
