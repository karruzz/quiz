/*
 * window.h
 *
 *  Created on: May 19, 2018
 *  Copyright © 2018-2081 Ilja Karasev <ilja.karasev@gmail.com>.
 *  All rights reserved.
 *     License: GNU GPL 3
 */

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

typedef view::LANGUAGE LAN;

enum CLR_SCHEME {
	BLUE = 1,
	WHITE,
	CYAN,
	GREEN,
	MAGENTA,
	RED,
	YELLOW,

	ERROR_WHITE,
	ERROR_BLACK,
	MISSED_BLACK
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
		WAIT, INPUT, OUTPUT
	};

	~Window() {
		remove();
	}

	Mode mode = Mode::WAIT;
	WINDOW *window = NULL;
	Geometry geometry = { -1 , -1 , -1 , -1 };

	void create();
	void remove();

	void waddstr_colored(const std::string& s, int color_scheme, bool bold = false);

public:
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
	CursorWindow(const std::function<void()>& resize)
		: resize_handle(resize)
	{}

	void focus(bool enable, bool show_cursor = true) {
		focused = enable;
		keypad(window, focused ? TRUE : FALSE);
		if (!focused) return;

		curs_set(show_cursor ? 1 : 0);
		wmove(window, cursor.y, cursor.x);
	}

	int get_key() {
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
	int tab_size;
	std::unique_ptr<Editor> editor;

	analysis::Verification verification;

	void update_window();
	void update_line();

public:
	AnswerWindow(const std::function<void()>& resize, int tab_size_)
		: CursorWindow(resize)
		, tab_size(tab_size_)
		, editor(new Editor(tab_size))
	{}

	virtual void refresh();
	void key_process(int ch);

	void prepare();
	std::list<std::string> get_lines();
	void show_analysed(const analysis::Verification& v);
};


class StatisticWindow : public Window
{
	Statistic statistic;

public:
	virtual void refresh();

	void update(Statistic s) {
		statistic = s;
		refresh();
	}
};


class QuestionWindow : public Window
{
	std::list<std::string> question;

public:
	virtual void refresh();

	void update(const Problem& p) {
		question = !p.inverted ? p.question :  p.solution;
		refresh();
	}
};


class SolutionWindow : public Window
{
	std::list<std::string> solution;
	bool visible = false;

public:
	virtual void refresh();

	void show(bool e) {
		visible = e;
		refresh();
	}

	void show_analysed(const analysis::Verification& v) {
		solution = v.solution;
	}

	void update(const Problem& p) {
		solution = !p.inverted ? p.solution : p.question;
		refresh();
	}
};


class MessageWindow : public Window
{
	std::string message;

	LAN language;

	std::string lang_to_str() {
		if (language == LAN::RU) return "RU";
		if (language == LAN::EN) return "EN";
		return std::string();
	}

public:
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
