#ifndef VIEWER_H
#define VIEWER_H

#include <iostream>
#include <string>

class Viewer {
public:
	enum color {
		RED = 31,
		GREEN = 32,
		YELLOW = 33,
		BLUE = 34,
		MAGNETTA = 35,
		CYAN = 36,
		RED_BACKGROUND = 41,
	};

	static void clear_screen();
	static void colored_line_output(const std::string& s, Viewer::color c, bool new_line = true);
};

#endif // VIEWER_H
