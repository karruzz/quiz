#include <ncurces_screen.h>
#include <ncursesw/ncurses.h>

#include <algorithm>
#include <functional>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>

#include <voice.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <functional>

#include <mutex>
#include <thread>

#define TAB_SIZE 4
#define COLOR_LIGHT_WHITE 15
#define COLOR_GRAY 8

namespace view {

namespace ncurses {

NScreen::NScreen(bool enter_accept_mode)
	: enter_accept_mode(enter_accept_mode)
{
	initscr();
	set_tabsize(TAB_SIZE);
	start_color();
	cbreak();
	noecho();

	init_pair(CYAN, 14, COLOR_GRAY);
	init_pair(RED, 9, COLOR_GRAY);
	init_pair(GREEN, 10, COLOR_GRAY);
	init_pair(YELLOW, 11, COLOR_GRAY);
	init_pair(BLUE, 12, COLOR_GRAY);
	init_pair(MAGENTA, 13, COLOR_GRAY);

	init_pair(COLOR::RED_BKGR, COLOR_LIGHT_WHITE, COLOR_RED);
	init_pair(BKGR, COLOR_LIGHT_WHITE, COLOR_GRAY);

	window_statistic.reset(new StatisticWindow());
	window_question.reset(new QuestionWindow());
	window_answer.reset(new AnswerWindow(std::bind(&NScreen::resize, this), TAB_SIZE));
	window_solution.reset(new SolutionWindow());
	window_message.reset(new MessageWindow());

	resize();
}

void NScreen::resize()
{
	const int statistic_h = 2, message_h = 1;
	int ques_h = (LINES - statistic_h - message_h) * 2 / 7;
	int answ_h = (LINES - statistic_h - ques_h - message_h) / 2;
	int resl_h =  LINES - statistic_h - ques_h - message_h - answ_h;

	window_statistic->resize({ .x = 0, .y = 0, .w = COLS, .h = statistic_h });
	window_question->resize({ .x = 0, .y = statistic_h, .w = COLS, .h = ques_h });
	window_answer->resize({ .x = 0, .y = statistic_h  + ques_h, .w = COLS, .h = answ_h });
	window_solution->resize({ .x = 0, .y = statistic_h + ques_h + answ_h, .w = COLS, .h = resl_h });
	window_message->resize({ .x = 0, .y = statistic_h + ques_h + answ_h + resl_h, .w = COLS, .h = message_h });

	window_answer->focus(true);
}

NScreen::~NScreen()
{
	endwin();
}

void NScreen::update_statistic(const Statistic &s)
{
	window_statistic->update(s);
}

void NScreen::show_problem(const Problem& problem)
{
	current_problem = problem;
	window_solution->update(problem);
	window_solution->show(false);
	window_question->update(problem);
	window_answer->prepare();
	window_answer->focus(true);
}

void NScreen::set_language(LANGUAGE language)
{
	window_message->set_lan(language);
}

std::tuple<Screen::INPUT_STATE, std::list<std::string>> NScreen::get_answer()
{
	window_message->update("F2 - check answer       F3 - skip question       F10 - exit ");

	for (;;) {
		int key = window_answer->get_key();

		if (key == KEY_F(2) || (enter_accept_mode && key == '\n'))
			return std::make_tuple(Screen::INPUT_STATE::ENTERED, window_answer->get_lines());
		else if (key == KEY_F(3))
			return std::make_tuple(Screen::INPUT_STATE::SKIPPED, std::list<std::string>());
		else if (key == KEY_F(10))
			return std::make_tuple(Screen::INPUT_STATE::EXIT, std::list<std::string>());
		else
			window_answer->key_process(key);
	}

	window_message->set_lan(LANGUAGE::UNKNOWN);
}

int NScreen::wait_pressed_key()
{
	window_answer->focus(true, false);
	return window_answer->get_key();
}

void NScreen::show_result(const analysis::Verification& v)
{
	window_answer->show_analysed(v);
}

void NScreen::show_solution()
{
	window_solution->show(true);
}

void NScreen::show_message(const std::string& msg)
{
	window_message->update(msg);
}

} // namespace ncurses

} // namespace view
