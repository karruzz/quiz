/*
 * parser.h
 *
 *  Created on: Feb 24, 2024
 *  Copyright © 2018-2081 Ilja Karasev <ilja.karasev@gmail.com>.
 *  All rights reserved.
 *     License: GNU GPL 3
 */

#ifndef OPTIONS_H
#define OPTIONS_H

#include <analyzer.h>

#include <string>

namespace cmd {

static const std::string HELP_MESSAGE =
	"-c    case unsensitive\n" \
	"-e    accept answer by enter key\n" \
	"-h    show this help\n" \
	"-i    invert questions and solutions, discard mixed mode (-m)\n" \
	"-l    input language auto-detect\n" \
	"-m    mixed mode, question and solution may be swapped\n" \
	"-p    play the solution\n" \
	"-q    not show question\n" \
	"-r    include to quiz problems, which were with errors last time only\n" \
	"-s    show statistic\n" \
	"-t    use topics\n" \
	"-u    punctuation unsensitive\n"
	"-w    play the question\n" \
	"-z    total recall\n";

} // namespace cmd

class Options
{
public:
	enum Flags {
		NONE,
		PLAY_SOLUTION,
		ACCEPT_BY_ENTER,
		QS_INVERTED,
		QS_MIXED,
		AUTO_LANGUAGE,
		REPEAT_ERRORS_ONLY,
		SHOW_STATISTICS,
		READ_QUESTION,
		HIDE_QUESTION,
		SHOW_CMD_HELP,
		USE_TOPICS,
		ANALYSIS_CASE_UNSENSITIVE,
		ANALYSIS_PUNTCTUATION_UNSENSITIVE,
		ANALYSIS_TOTAL_RECALL,
	};

	bool parse_arguments(int argc, char* argv[]);
	bool get(Flags flag) const;
	std::list<std::string> args(Flags flag) const;

	bool help() const { return _show_help; }
	const std::string& filename() const { return _filename; }

private:
	static std::map<char, Flags > args_lookup_table;

	std::map<Flags, std::list<std::string> > _args;
	std::string _filename;
	bool _show_help;
};

#endif // OPTIONS_H
