#ifndef EDITOR_H
#define EDITOR_H

#include <vector>
#include <list>
#include <string>

class Editor
{
	const size_t tab_width;
	const size_t ascii_width;
	const size_t utf8_width;

	std::vector<std::string> lines;
	// position of line's every symbol on screen, needs to handle tabs and utf-8 paired symbols
	std::vector<std::vector<size_t>> screen_positions;

	size_t cursor_x, cursor_y;

	bool is_ascii(char c);
	size_t sym_width(char c);

	void put_cursor_inside_line();
	void update_screen_positions(size_t x, size_t y);

public:
	explicit Editor(size_t tab_size);

	bool backspace();
	bool del();
	void home();
	void end();
	void page_up();
	void page_down();
	void right();
	void left();
	void up();
	void down();
	void new_line();
	void add_ch(char ch);
	void add_ch(char ch_high, char ch_low);
	void add_str(const std::string &s);

	const std::vector<std::string>& get_lines();
	std::list<std::string> get_lines_list();
	const std::string& get_current_line();
	size_t get_screen_x();
	size_t get_screen_y();
};

#endif // EDITOR_H
