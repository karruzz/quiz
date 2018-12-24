#include <window.h>

namespace view {

namespace ncurses {

#define SERVICE_COLOR YELLOW

void Window::create()
{
	window = newwin(geometry.h, geometry.w, geometry.y, geometry.x);
	wattron(window, COLOR_PAIR(BKGR));
	wbkgd(window, COLOR_PAIR(BKGR));
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
	for (const std::string s: question)
		mvwaddstr(window, y++, 0, s.c_str());

	wrefresh(window);
}

void SolutionWindow::refresh()
{
	clear();
	if (!visible) return;

	int y = 0;
	for (const std::string s: solution)
		mvwaddstr(window, y++, 0, s.c_str());

	wrefresh(window);
}

void MessageWindow::refresh()
{
	clear();
	wmove(window, 0, 1);
	waddstr_colored(message, SERVICE_COLOR);

	std::string lan = lan_to_str();
	wmove(window, 0, geometry.w - lan.size() - 1);
	waddstr_colored(lan, SERVICE_COLOR);
	wrefresh(window);
}

void AnswerWindow::update_screen()
{
	clear();
	const std::vector<std::string>& answer = editor->get_lines();

	size_t y = 0;
	if (mode == Mode::INPUT) {
		for (const std::string s: answer)
			mvwaddstr(window, y++, 0, s.c_str());
	} else if (mode == Mode::OUTPUT) {
		static auto mvwaddstr_colored = [&](int y, int x, const char *str, int color_scheme) {
			wmove(window, y, x);
			return waddstr_colored(str, color_scheme);
		};

		for (auto line_it = verification.answer.begin()
			; line_it != verification.answer.end(); ++line_it, ++y) {
			std::string line = *line_it;

			mvwaddstr(window, y, 0, line.c_str());
			const auto errors_map_it = verification.errors.find(y);
			if (errors_map_it != verification.errors.end()) {
				const std::map<int, std::string>& errors_map = errors_map_it->second;
				for (auto errors_it = errors_map.cbegin(); errors_it != errors_map.cend(); ++errors_it) {
					mvwaddstr_colored(y, errors_it->first, errors_it->second.c_str(), RED_BKGR);
				}

//				size_t error_since_sym = static_cast<size_t>(error_it->second);
//				if (error_since_sym < line_it->size()) {
//					mvwaddstr(window, y, 0, line_it->substr(0, error_since_sym).c_str());
//					waddstr_colored(line_it->substr(error_since_sym), RED_BKGR);
//				} else {
//					mvwaddstr(window, y, 0, line_it->c_str());
//					waddstr_colored(" ", RED_BKGR);
//				}
			}
		}

		++y;
		if (verification.state == analysis::MARK::RIGHT) {
			mvwaddstr_colored(y, 0, "[right]", SERVICE_COLOR);
		} else if (verification.state == analysis::MARK::INVALID_LINES_NUMBER) {
			mvwaddstr_colored(y, 0, "[invalid lines amount]", SERVICE_COLOR);
		} else if (verification.state == analysis::MARK::ERROR) {
			mvwaddstr_colored(y, 0, "[invalid answer]", SERVICE_COLOR);
		} else if (verification.state == analysis::MARK::NOT_FULL_ANSWER) {
			mvwaddstr_colored(y, 0, "[not full answer]", SERVICE_COLOR);
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
	update_screen();
	wmove(window, editor->get_screen_y(), editor->get_screen_x());
	wrefresh(window);
}

void AnswerWindow::key_process(int key)
{
	switch (key) {
		case KEY_BACKSPACE:
			editor->backspace() ? update_screen() : update_line();
			break;
		case KEY_DC:
			editor->del() ? update_screen() : update_line();
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
	mode = Mode::INPUT;
	editor.reset(new Editor(tab_size));
	update_cursor({ editor->get_screen_x(), editor->get_screen_y() });
	update_screen();
}

void AnswerWindow::show_analysed(const analysis::Verification& v) {
	mode = Mode::OUTPUT;
	verification = v;
	update_screen();
}

std::list<std::string> AnswerWindow::get_lines()
{
	return editor->get_lines_list();
}

} // namespace ncurses

} // namespace view
