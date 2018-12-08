#ifndef WINDOW_H
#define WINDOW_H

#include <functional>
#include <memory>

#include <ncursesw/ncurses.h>

#include <editor.h>
#include <voice.h>
#include <viewer.h>

namespace view {

namespace ncurses {

enum COLOR {
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
	enum class Mode {
		WAIT,
		INPUT,
		OUTPUT
	};

	Mode mode;
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

	std::function<void()> resize_handle;

protected:
	void update_cursor(Point position) { cursor = position; }

public:
	CursorWindow(const Geometry &g, const std::function<void()>& resize)
		: Window(g)
		, resize_handle(resize)
	{}

	void focus(bool enable, bool show_cursor = true) {
		focused = enable;
		keypad(window, focused ? TRUE : FALSE);
		if (!focused) return;

		curs_set(show_cursor ? 1 : 0);
		wmove(window, cursor.y, cursor.x);
	}

	int get_key()
	{
		for (;;) {
			int key = wgetch(window);
			if (key == ERR)
				throw std::runtime_error("CursorWindow: wgetch error");
			if (key == KEY_RESIZE)
				resize_handle();
			else
				return key;
		}
	}
};

class AnswerWindow : public CursorWindow
{
	AudioRecord audio_record;
	std::unique_ptr<Editor> editor;
	int tab_size;
//	std::map<int, std::list<int>> errors;

	analysis::Verification verification;

	void update_screen();
	void update_line();
public:
	AnswerWindow(const Geometry &g, const std::function<void()>& resize, int tab_size_)
		: CursorWindow(g, resize)
		, tab_size(tab_size_)
	{ }

	virtual void refresh();
	void key_process(int ch);

	void prepare();
	std::list<std::string> get_lines();
	void show_analysed(const analysis::Verification& v);
};

class StatisticWindow : public Window {
	Statistic statistic;
public:
	StatisticWindow(Geometry g)
		: Window(g)
	{}

	virtual void refresh();

	void update_statistic(Statistic s) {
		statistic = s;
		refresh();
	}
};

class QuestionWindow : public Window {
	std::list<std::string> question;
public:
	QuestionWindow(Geometry g)
		: Window(g)
	{}

	virtual void refresh();

	void update(const Problem& p) {
		question = p.inverted ? p.question :  p.solution;
		refresh();
	}
};

class SolutionWindow : public Window {
	std::list<std::string> solution;

public:
	SolutionWindow(Geometry g)
		: Window(g)
	{}

	virtual void refresh();

	void show(bool e)
	{
		mode = e ? Mode::OUTPUT : Mode::WAIT;
		refresh();
	}

	void update(const Problem& p)
	{
		solution = !p.inverted ? p.solution : p.question;
		refresh();
	}
};

typedef view::LANGUAGE LAN;

class MessageWindow : public Window
{
	std::string message;

	LAN language;

	std::string lan_to_str() {
		if (language == LAN::RU) return "RU";
		if (language == LAN::EN) return "EN";
		return std::string();
	}

public:
	MessageWindow(Geometry g)
		: Window(g)
		, language(LAN::UNKNOWN)
	{}

	virtual void refresh();

	void set_lan (LAN lan) {
		language = lan;
	}

	void update(const std::string &m) {
		message = m;
		refresh();
	}
};

} // namespace ncurses

} // namespace view

#endif // WINDOW_H
