#include "viewer.h"

void Viewer::clear_screen() {
	std::cout << "\033[2J\033[1;1H";
}

void Viewer::colored_line_output(const std::string& s, Viewer::color c, bool new_line) {
	std::cout << "\033[1;" << c << "m" << s << "\033[0m";
	if (new_line)
		std::cout << std::endl;
}

