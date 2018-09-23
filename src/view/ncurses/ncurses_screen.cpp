#include <ncurces_screen.h>
#include <ncursesw/ncurses.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <memory>

#include <record.h>
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

NScreen::NScreen(bool show_right_answer)
	: show_right_answer(show_right_answer)
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

	init_pair(RED_BKGR, COLOR_LIGHT_WHITE, COLOR_RED);
	init_pair(BKGR, COLOR_LIGHT_WHITE, COLOR_GRAY);

	update_geometry();
	window_statistic = std::unique_ptr<StatisticWindow>(new StatisticWindow(geometry_statistic));
	window_question = std::unique_ptr<QuestionWindow>(new QuestionWindow(geometry_question));
	window_answer = std::unique_ptr<AnswerWindow>(new AnswerWindow(geometry_answer, TAB_SIZE));
	window_result = std::unique_ptr<ResultWindow>(new ResultWindow(geometry_result));
	window_message = std::unique_ptr<MessageWindow>(new MessageWindow(geometry_message));
}

void NScreen::update_geometry() {
	const int statistic_h = 2, message_h = 1;
	int ques_h = (LINES - statistic_h - message_h) * 2 / 7;
	int answ_h = (LINES - statistic_h - ques_h - message_h) / 2;
	int resl_h =  LINES - statistic_h - ques_h - message_h - answ_h;

	geometry_statistic = Geometry { .x = 0, .y = 0, .w = COLS, .h = statistic_h };
	geometry_question = Geometry { .x = 0, .y = statistic_h, .w = COLS, .h = ques_h };
	geometry_answer = Geometry { .x = 0, .y = statistic_h  + ques_h, .w = COLS, .h = answ_h };
	geometry_result = Geometry { .x = 0, .y = statistic_h + ques_h + answ_h, .w = COLS, .h = resl_h };
	geometry_message = Geometry { .x = 0, .y = statistic_h + ques_h + answ_h + resl_h, .w = COLS, .h = message_h };
}

void NScreen::resize_handle()
{
	update_geometry();
	window_statistic->resize(geometry_statistic);
	window_question->resize(geometry_question);
	window_answer->resize(geometry_answer);
	window_result->resize(geometry_result);
	window_message->resize(geometry_message);

	window_answer->focus(true);
}

NScreen::~NScreen()
{
	endwin();
}

void NScreen::update_statistic(const Statistic &s)
{
	window_statistic->update_statistic(s);
}

void NScreen::show_question(const std::list<std::string> &question)
{
	window_question->update_question(question);
	window_result->enable(false);
}

std::tuple<Screen::INPUT_STATE, std::list<std::string>> NScreen::get_answer()
{
	window_answer->prepare();
	window_message->update(" F2 - check answer       F3 - skip question       F4 - capture audio       F10 - exit");
	window_answer->focus(true);

	for (;;) {
		int key = window_answer->getkey();

		if (key == ERR)
			throw std::runtime_error("wgetch error");
		else if (key == KEY_F(2))
			return std::make_tuple(Screen::INPUT_STATE::ENTERED, window_answer->get_answer());
		else if (key == KEY_F(3))
			return std::make_tuple(Screen::INPUT_STATE::SKIPPED, std::list<std::string>());
		else if (key == KEY_F(10))
			return std::make_tuple(Screen::INPUT_STATE::EXIT, std::list<std::string>());
		else if (key == KEY_RESIZE)
			resize_handle();
		else
			window_answer->key_pressed(key);
	}
}

void NScreen::show_result(
		Screen::CHECK_STATE state,
		const std::list<std::string> &answer,
		const std::map<int, int> &errors)
{
	window_result->update(state, answer, errors);
	window_result->enable(true);
	window_message->update(" Press any key ... ");
	window_answer->focus(true);

	for (;;) {
		int key = window_answer->getkey();
		if (key == ERR)
			throw std::runtime_error("wgetch error");
		else if (key == KEY_RESIZE)
			resize_handle();
		else
			return;
	}
}

} // namespace ncurses

} // namespace view
