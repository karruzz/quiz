/*
 * ncurses_screen.cpp
 *
 *  Created on: Sep 23, 2018
 *  Copyright © 2018-2081 Ilja Karasev <ilja.karasev@gmail.com>.
 *  All rights reserved.
 *     License: GNU GPL 3
 */

#include <ncurces_screen.h>

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
//#include <sys/ioctl.h>
#include <functional>

#include <mutex>
#include <thread>

#define LIGHT_MODE 1

namespace view {

namespace ncurses {

NScreen::NScreen(bool enter_accept_mode, int tab_size)
	: enter_accept_mode(enter_accept_mode)
{
	enum CLR_CODE {
		BLACK = 0,
		DARK_GREEN = 2,
		DARK_CYAN = 6,
		LIGHT_RED = 1,
		GRAY = 8,
		RED = 9,
		GREEN = 10,
		//YELLOW = 11, // dark mode
		YELLOW = 6, // light mode
		BLUE = 12,
		MAGENTA = 13,
		CYAN = 14,
		LIGHT_WHITE = 15,
		DARK_BLACK = 16,
	};

	initscr();
	set_tabsize(tab_size);
	start_color();
	cbreak();
	noecho();

#ifdef LIGHT_MODE
	int bkgr = CLR_CODE::LIGHT_WHITE;
	int bkgr_error = CLR_CODE::RED;
	int bkgr_missed = CLR_CODE::YELLOW;
#else
	int bkgr = CLR_CODE::GRAY;
	int bkgr_error = CLR_CODE::LIGHT_RED;
	int bkgr_missed = CLR_CODE::YELLOW;
#endif

	init_pair(CLR_SCHEME::WHITE, CLR_CODE::LIGHT_WHITE, bkgr);
	init_pair(CLR_SCHEME::GRAY, CLR_CODE::BLACK, bkgr);

#ifdef LIGHT_MODE
	init_pair(CLR_SCHEME::CYAN, CLR_CODE::DARK_CYAN, bkgr);
#else
	init_pair(CLR_SCHEME::CYAN, CLR_CODE::CYAN, bkgr);
#endif

	init_pair(CLR_SCHEME::RED, CLR_CODE::RED, bkgr);
#ifdef LIGHT_MODE
	init_pair(CLR_SCHEME::GREEN, CLR_CODE::DARK_GREEN, bkgr);
#else
	init_pair(CLR_SCHEME::GREEN, CLR_CODE::GREEN, bkgr);
#endif
	init_pair(CLR_SCHEME::YELLOW, CLR_CODE::YELLOW, bkgr);

	init_pair(CLR_SCHEME::BLUE, CLR_CODE::BLUE, bkgr);
	init_pair(CLR_SCHEME::MAGENTA, CLR_CODE::MAGENTA, bkgr);

	init_pair(CLR_SCHEME::ERROR_WHITE, CLR_CODE::LIGHT_WHITE, bkgr_error);
	init_pair(CLR_SCHEME::ERROR_BLACK, CLR_CODE::DARK_BLACK, bkgr_error);
	init_pair(CLR_SCHEME::MISSED_BLACK, CLR_CODE::DARK_BLACK, bkgr_missed);

	window_statistic.reset(new StatisticWindow());
	window_question.reset(new QuestionWindow());
	window_answer.reset(new AnswerWindow(std::bind(&NScreen::resize, this), tab_size));
	window_solution.reset(new SolutionWindow());
	window_message.reset(new MessageWindow());

	resize();
}

void NScreen::resize()
{
	const int statistic_h = 2, message_h = 1;
	int ques_h = (LINES - statistic_h - message_h) * 2 / 7 - 1;
	int answ_h = (LINES - statistic_h - ques_h - message_h) / 2 + 1;
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

void NScreen::update_statistic(const Statistics &s)
{
	window_statistic->update(s);
}

void NScreen::show_problem(const Problem& problem)
{
	current_problem = problem;
	window_solution->update(problem);
	window_solution->visibility(false);
	window_question->update(problem);
	window_answer->prepare();
	window_answer->focus(true);
	window_message->update("F2 - check answer       F3 - skip question       F12 - exit ");
}

void NScreen::set_language(utils::Language language)
{
	window_message->set_lan(language);
}

std::tuple<Screen::INPUT_STATE, std::list<std::string>> NScreen::get_answer()
{
	for (;;) {
		int key = window_answer->get_key();

		if (key == KEY_F(2) || (enter_accept_mode && key == '\n'))
			return std::make_tuple(Screen::INPUT_STATE::ENTERED, window_answer->get_lines());
		else if (key == KEY_F(3))
			return std::make_tuple(Screen::INPUT_STATE::SKIPPED, std::list<std::string>());
		else if (key == KEY_F(12))
			return std::make_tuple(Screen::INPUT_STATE::EXIT, std::list<std::string>());
		else
			window_answer->key_process(key);
	}

	window_message->set_lan(utils::Language::UNKNOWN);
}

int NScreen::wait_pressed_key()
{
	window_answer->focus(true, false);
	int key = window_answer->get_key();
	if (key == KEY_F(3)) return FKEY::F3;
	return key;
}

void NScreen::show_result(const analysis::Verification& v)
{
	window_solution->visibility(true);
	window_answer->show_analysed(v);
}

void NScreen::show_solution()
{
	window_solution->visibility(true);
}

void NScreen::show_message(const std::string& msg)
{
	window_message->update(msg);
}

} // namespace ncurses

} // namespace view
