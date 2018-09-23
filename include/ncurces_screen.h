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
	explicit NScreen(bool show_right_answer);
	virtual ~NScreen();

	virtual void update_statistic(const Statistic &s);
	virtual void show_question(const std::list<std::string> &question);
	virtual std::tuple<Screen::INPUT_STATE, std::list<std::string>> get_answer();
	virtual void show_result(
		Screen::CHECK_STATE state,
		const std::list<std::string> &right_answer,
		const std::map<int, int> &error);

private:
	bool show_right_answer;

	std::unique_ptr<StatisticWindow> window_statistic;
	std::unique_ptr<QuestionWindow>  window_question;
	std::unique_ptr<ResultWindow>    window_result;
	std::unique_ptr<MessageWindow>   window_message;
	std::unique_ptr<AnswerWindow>    window_answer;

	Geometry geometry_statistic;
	Geometry geometry_question;
	Geometry geometry_answer;
	Geometry geometry_result;
	Geometry geometry_message;

	// for update
	void resize_handle();
	void update_geometry();

	Statistic current_statistic;
};

} // namespace ncurses

} // namespace view

#endif // NCURCES_SCREEN_H
