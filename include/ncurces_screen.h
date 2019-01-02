/*
 * ncurces_screen.h
 *
 *  Created on: Sep 23, 2018
 *  Copyright © 2018-2081 Ilja Karasev <ilja.karasev@gmail.com>.
 *  All rights reserved.
 *     License: GNU GPL 3
 */

#ifndef NCURCES_SCREEN_H
#define NCURCES_SCREEN_H

#include <tuple>
#include <map>
#include <memory>

#include <ncursesw/ncurses.h>

#include <viewer.h>
#include <window.h>

namespace view {

namespace ncurses {

class NScreen : public Screen
{
public:
	explicit NScreen(bool enter_accept_mode);
	virtual ~NScreen();

	virtual std::tuple<Screen::INPUT_STATE, std::list<std::string>> get_answer();
	virtual int wait_pressed_key();

	virtual void set_language(LANGUAGE layout);
	virtual void update_statistic(const Statistic &s);
	virtual void show_problem(const Problem& problem);
	virtual void show_result(const analysis::Verification& v);
	virtual void show_solution();
	virtual void show_message(const std::string& s);

private:
	bool enter_accept_mode;

	std::unique_ptr<StatisticWindow> window_statistic;
	std::unique_ptr<QuestionWindow>  window_question;
	std::unique_ptr<SolutionWindow>  window_solution;
	std::unique_ptr<MessageWindow>   window_message;
	std::unique_ptr<AnswerWindow>    window_answer;

	void resize();

	Statistic current_statistic;
	Problem current_problem;
};

} // namespace ncurses

} // namespace view

#endif // NCURCES_SCREEN_H
