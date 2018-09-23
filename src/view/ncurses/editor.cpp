#include <editor.h>

bool Editor::is_ascii(char c) { return c > 0; }
size_t Editor::sym_width(char c) { return is_ascii(c) ? ascii_width : utf8_width; }

void Editor::put_cursor_inside_line()
{
	size_t line_end = lines[cursor_y].size();
	if (cursor_x > line_end) cursor_x = line_end;
}

void Editor::update_screen_positions(size_t x, size_t y)
{
	if (screen_positions[y].size() != lines[y].size())
		screen_positions[y].resize(lines[y].size());

	size_t screen_x = (x > 0) ? screen_positions[y][x - 1] : 0;
	while(x < lines[y].size()) {
		if (lines[y][x] == '\t') {
			size_t next = screen_x + tab_width;
			screen_x = next - next % tab_width;
		} else
			++screen_x;

		size_t w = sym_width(lines[y][x]);
		for (size_t j = 0; j < w; ++j)
			screen_positions[y][x + j] = screen_x;

		x += w;
	}
}

Editor::Editor(size_t tab_size)
	: tab_width(tab_size)
	, ascii_width(1)
	, utf8_width(2)
	, cursor_x(0)
	, cursor_y(0)
{
	lines = std::vector<std::string>(1);
	screen_positions = std::vector<std::vector<size_t>>(1);
}

void Editor::backspace() {
	if (cursor_x != 0) {
		size_t width = sym_width(lines[cursor_y][cursor_x - 1]);
		cursor_x -=  width;
		lines[cursor_y].erase(cursor_x, width);
	} else { // begin of the line
		if (cursor_y == 0) return;
		std::string subst = lines[cursor_y];
		lines.erase(lines.begin() + cursor_y);
		screen_positions.erase(screen_positions.begin() + cursor_y);
		--cursor_y;
		cursor_x = lines[cursor_y].size();
		lines[cursor_y].append(subst);
	}

	update_screen_positions(cursor_x, cursor_y);
}

void Editor::del() {
	if (cursor_x != lines[cursor_y].size()) {
		size_t width = sym_width(lines[cursor_y][cursor_x]);
		lines[cursor_y].erase(cursor_x, width);
	} else { // end of line
		if (cursor_y == lines.size() - 1) return;
		std::string subst = lines[cursor_y + 1];
		lines.erase(lines.begin() + cursor_y + 1);
		screen_positions.erase(screen_positions.begin()  + cursor_y + 1);
		lines[cursor_y].append(subst);
	}

	update_screen_positions(cursor_x, cursor_y);
}

void Editor::home() { cursor_x = 0; }

void Editor::end() { cursor_x = lines[cursor_y].size(); }

void Editor::page_up() {
	cursor_y = 0;
	put_cursor_inside_line();
}

void Editor::page_down() {
	cursor_y = lines.size() - 1;
	put_cursor_inside_line();
}

void Editor::right() {
	if (cursor_x >= lines[cursor_y].size()) return;
	cursor_x += sym_width(lines[cursor_y][cursor_x]);
}

void Editor::left() {
	if (cursor_x == 0) return;
	cursor_x -= sym_width(lines[cursor_y][cursor_x - 1]);
}

void Editor::up() {
	if (cursor_y == 0) return;
	--cursor_y;
	put_cursor_inside_line();
}

void Editor::down() {
	if (cursor_y == lines.size() - 1) return;
	++cursor_y;
	put_cursor_inside_line();
}

void Editor::new_line() {
	std::string &&remaining_subst = lines[cursor_y].substr(cursor_x);
	lines[cursor_y].erase(cursor_x);
	update_screen_positions(cursor_x, cursor_y);

	++cursor_y;
	lines.insert(lines.begin() + cursor_y, remaining_subst);
	screen_positions.insert(screen_positions.begin() + cursor_y, std::vector<size_t>());
	cursor_x = 0;
	update_screen_positions(cursor_x, cursor_y);
}

void Editor::add_ch(char ch) {
	lines[cursor_y].insert(cursor_x, 1, ch);
	update_screen_positions(cursor_x, cursor_y);
	cursor_x += ascii_width;
}

void Editor::add_ch(char ch_high, char ch_low) {
	lines[cursor_y].insert(cursor_x, 1, ch_low);
	lines[cursor_y].insert(cursor_x, 1, ch_high);
	update_screen_positions(cursor_x, cursor_y);
	cursor_x += utf8_width;
}

void Editor::add_str(const std::string &s) {
	lines[cursor_y].insert(cursor_x, s);
	update_screen_positions(cursor_x, cursor_y);
	cursor_x += s.length();
}

const std::vector<std::string>& Editor::get_lines() { return lines; }

std::list<std::string> Editor::get_lines_list() {
	std::list<std::string> result;
	std::copy( lines.begin(), lines.end(), std::back_inserter( result ) );
	return result;
}

const std::string& Editor::get_current_line()
{
	return lines[cursor_y];
}

size_t Editor::get_screen_x() { return cursor_x > 0 ? screen_positions[cursor_y][cursor_x - 1] : 0; }
size_t Editor::get_screen_y() { return cursor_y; }
