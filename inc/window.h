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
#include <quiz.h>
#include <voice.h>
#include <viewer.h>

namespace view {

namespace ncurses {

enum CLR_SCHEME {
	BLUE = 1,
	WHITE,
	CYAN,
	GREEN,
	MAGENTA,
	RED,
	YELLOW,
	GRAY,

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

	std::function<void()> resize_handle;

protected:
	void update_cursor(Point position) { cursor = position; }

public:
	CursorWindow(const std::function<void()>& resize)
		: resize_handle(resize)
	{}

	void focus(bool focused, bool show_cursor = true) {
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
	Statistics statistics;

public:
	virtual void refresh();

	void update(Statistics s) {
		statistics = s;
		refresh();
	}
};


class QuestionWindow : public Window
{
	std::list<std::string> question;

public:
	virtual void refresh();

	void update(const Problem& p) {
		if (p.not_show_question)
			question.clear();
		else
			question = p.question();
		refresh();
	}
};


class SolutionWindow : public Window
{
	std::list<std::string> solution;
	bool visible = false;

public:
	virtual void refresh();

	void visibility(bool e) {
		visible = e;
		refresh();
	}

	void update(const Problem& p) {
		solution = p.solution();
		refresh();
	}
};


class MessageWindow : public Window
{
	std::string message;

	utils::Language language;

	std::string lang_to_str() {
		if (language == utils::Language::RU) return "RU";
		if (language == utils::Language::EN) return "EN";
		return std::string();
	}

public:
	virtual void refresh();

	void set_lan (utils::Language lan) {
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
