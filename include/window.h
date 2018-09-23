#ifndef WINDOW_H
#define WINDOW_H

#include <functional>
#include <memory>

#include <ncursesw/ncurses.h>

#include <editor.h>
#include <record.h>
#include <viewer.h>

namespace view {

namespace ncurses {

enum Color {
	RED = 1,
	GREEN,
	YELLOW,
	BLUE,
	MAGENTA,
	CYAN,
	RED_BKGR,
	BKGR
};

struct Point {
	size_t x, y;
};

struct Geometry {
	int x, y, w, h;
};

class Window
{
protected:
	WINDOW *window;
	Geometry geometry;

	void create();
	void remove();

	void waddstr_colored(const std::string &s, int color_scheme);

public:
	Window(const Geometry &g);
	virtual ~Window();

	virtual void clear();
	virtual void refresh();
	virtual void resize(const Geometry g);
};

class CursorWindow : public Window
{
	Point cursor = { 0, 0 };
	bool focused;

protected:
	void update_cursor(Point position) { cursor = position; }

public:
	CursorWindow(const Geometry &g) : Window(g) {}

	void focus(bool enable) {
		focused = enable;
		keypad(window, focused ? TRUE : FALSE);
		if (focused)
			wmove(window, cursor.y, cursor.x);
	}

	int getkey() { return wgetch(window); }
	void virtual key_pressed(int ch) = 0;

	void add_resize_handler(std::function<void()> cb) {
		resize_callback = cb;
	}

private:
	std::function<void()> resize_callback;
};

class AnswerWindow : public CursorWindow
{
	AudioRecord audio_record;
	std::unique_ptr<Editor> editor;
	int tab_size;

	void update_screen();
	void update_line();
public:
	AnswerWindow(const Geometry &g, int tab_size_) :
		CursorWindow(g),
		tab_size(tab_size_)
	{ }

	virtual void refresh();
	void virtual key_pressed(int ch);

	void prepare();
	std::list<std::string> get_answer();
};

class StatisticWindow : public Window {
	Statistic statistic;
public:
	StatisticWindow(Geometry g) : Window(g) {}

	virtual void refresh();

	void update_statistic(Statistic s) {
		statistic = s;
		refresh();
	}
};

class QuestionWindow : public Window {
	std::list<std::string> question;
public:
	QuestionWindow(Geometry g) : Window(g) {}

	virtual void refresh();

	void update_question(const std::list<std::string> &q) {
		question = q;
		refresh();
	}
};

class ResultWindow : public Window {
	bool enable_result;
	bool show_right_answer;

	Screen::CHECK_STATE state;
	std::list<std::string> answer;
	std::map<int, int> errors;

public:
	ResultWindow(Geometry g)
		: Window(g)
		, enable_result(false)
		, show_right_answer(true)
	{
		keypad(window, TRUE);
	}

	virtual void refresh();

	void enable(bool e) {
		enable_result = e;
		refresh();
	}

	void update(
		Screen::CHECK_STATE state_,
		const std::list<std::string> &answer_,
		const std::map<int, int> &errors_)
	{
		state = state_;
		answer = answer_;
		errors = errors_;
		refresh();
	}
};

class MessageWindow : public Window {
	std::string message;
public:
	MessageWindow(Geometry g) : Window(g) {}

	virtual void refresh();

	void update(const std::string &m) {
		message = m;
		refresh();
	}
};

} // namespace ncurses

} // namespace view

#endif // WINDOW_H
