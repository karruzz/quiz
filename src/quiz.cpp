/*
 * quiz.cpp
 *
 *  Created on: May 19, 2018
 *  Copyright © 2018-2081 Ilja Karasev <ilja.karasev@gmail.com>.
 *  All rights reserved.
 *     License: GNU GPL 3
 */

// learn.cpp : Defines the entry point for the console application.
//

#include <iostream>

#include <vector>
#include <map>

#include <stddef.h>

#include <iterator>
#include <string>
#include <sstream>
#include <random>
#include <algorithm>
#include <memory>
#include <ctime>
#include <cctype>
#include <tuple>

#include <locale.h>

#include "log.h"
#include "viewer.h"
#include "ncurces_screen.h"
#include "problem.h"
#include "parser.h"
#include "analyzer.h"

#define SHOW_HELP          "-h"
#define PLAY_SOLUTION      "-p"
#define NOT_SHOW_QUESTION  "-q"
#define MIXED_MODE         "-m"
#define INVERT_MODE        "-i"
#define SHOW_STATISTIC     "-s"
#define REPEAT_ERRORS      "-r"
#define LANGUAGE_RECOGNIZE "-l"
#define ENTER_MODE         "-e"
#define NUMBERS            "-n"
#define CASE_UNSENSETIVE   "-c"
#define PUNCT_UNSENSETIVE  "-u"
#define TOTAL_RECALLD      "-z"
#define READ_QUESTION      "-w"

// todo: voice refactor
//       log debug to window_debug, which could be hidden

const char * help_message =
	"-h	show this help\n" \
	"-p	play the solution\n" \
	"-q	not show question\n" \
	"-m	mixed mode, question and solution may be swapped\n" \
	"-i	invert questions and solutions, discard mixed mode (-m)\n" \
	"-s	show statistic\n" \
	"-r	include to quiz problems, which were with errors last time only\n" \
	"-l	input language auto-detect\n" \
	"-e	accept answer by enter key\n" \
	"-t	use topics\n" \
	"-c	case unsensitive\n" \
	"-u	punctuation unsensitive\n"
	"-z	total recall\n" \
	"-w	read question but not show it\n";

namespace {

namespace an = analysis;

typedef utils::LANGUAGE LAN;

const int REPEAT_TIMES = 2;
const int TAB_SIZE = 4;

void set_os_lang(LAN language)
{
	if (language == LAN::RU)
		system("setxkbmap -layout ru,us -option grp:alt_shift_toggle");
	else if (language == LAN::EN)
		system("setxkbmap -layout us,ru -option grp:alt_shift_toggle");
}

} // namespace

int main(int argc, char* argv[])
{
	if (argc < 2) {
		logging::Error() << "argv is empty - no input file name" << logging::endl;
		return 1;
	}

	setlocale(LC_ALL, "");

	std::string problems_filename(argv[1]);

	// split params to keys and values
	std::map<std::string, std::vector<std::string> > params;
	std::string key;
	for (int i = 2; i < argc; i++) {
		std::string param = argv[i];
		if (param[0] == '-') {
			// split "-abc" to "-a" "-b" "-c"
			param.erase(0, 1);
			for (char c: param) {
				key = "-" + std::string(1, c);
				params[key] = std::vector<std::string>();
			}
			continue;
		}

		if (key.empty()) {
			logging::Error() << "arguments without key";
			return 1;
		}

		params.at(key).push_back(param);
	}

	if (problems_filename == "-h" || params.find(SHOW_HELP) != params.end()) {
		logging::Message() << help_message;
		return 0;
	}

	bool play_solution = params.find(PLAY_SOLUTION) != params.end();
	bool enter_accept_mode = params.find(ENTER_MODE) != params.end();
	bool invert_mode = params.find(INVERT_MODE) != params.end();
	bool mixed_mode = params.find(MIXED_MODE) != params.end() && !invert_mode;
	bool auto_language = params.find(LANGUAGE_RECOGNIZE) != params.end();
	bool repeat_errors_only = params.find(REPEAT_ERRORS) != params.end();
	bool show_statistic = params.find(SHOW_STATISTIC) != params.end();
	bool read_question = params.find(READ_QUESTION) != params.end();
	bool not_show_question = params.find(NOT_SHOW_QUESTION) != params.end();

	int analyze_flags = an::Analyzer::NONE;
	if (params.find(CASE_UNSENSETIVE) != params.end())
		analyze_flags |= an::Analyzer::OPTIONS::CASE_UNSENSITIVE;

	if (params.find(PUNCT_UNSENSETIVE) != params.end())
		analyze_flags |= an::Analyzer::OPTIONS::PUNCT_UNSENSITIVE;

	if (params.find(TOTAL_RECALLD) != params.end())
		analyze_flags |= an::Analyzer::OPTIONS::TOTAL_RECALL;

	std::vector<Problem> problems = Parser::load(problems_filename, params);

	if (show_statistic) {
		logging::Message msg;
		for (const Problem& p: problems) {
			msg << "? ";
			for (const std::string& q: p.question)
				msg << q << logging::endl;

			msg << "> ";
#ifdef DEBUG
			msg << "hash: " << p.question_hash << "; ";
#endif
			msg << "total errors: " << p.total_errors << "; " ;
			msg << "last errors: " << p.last_errors << ";" << logging::endl;
		}
		return 0;
	}

	std::random_device rd;  // used to obtain a seed for the random number engine
	std::mt19937 generator(rd()); // standard mersenne_twister_engine seeded with rd()

	std::vector<int> to_solve;
	for (size_t i = 0; i < problems.size(); ++i) {
		if (repeat_errors_only && problems[i].last_errors == 0)
			continue;

		to_solve.push_back(static_cast<int>(i));
	}

	view::ncurses::NScreen screen(enter_accept_mode, TAB_SIZE);
	int errors_count = 0, solved_count = 0, solving_num = -1, previous_solving_num = -1;

	auto update_statistic = [&]() {
		view::Statistic statistic = {
			static_cast<int>(to_solve.size()),
			solved_count,
			errors_count,
			problems.at(solving_num).repeat,
			problems.at(solving_num).errors,
			problems.at(solving_num).total_errors
		};
		screen.update_statistic(statistic);
	};

	an::Analyzer analyzer;
	while (to_solve.size() != 0) {
		do {
			std::uniform_int_distribution<> distribution (0, to_solve.size() - 1);
			solving_num = to_solve[distribution(generator)];
		} while (to_solve.size() > 1 && solving_num == previous_solving_num);
		previous_solving_num = solving_num;

		Problem& problem = problems[solving_num];
		problem.inverted = invert_mode;
		problem.not_show_question = not_show_question;

		if (mixed_mode) {
			static std::uniform_int_distribution<> distribution (0, 1);
			problem.inverted = distribution(generator) == 0;
		}

		auto q = problem.inverted ? problem.solution : problem.question;
		auto s = problem.inverted ? problem.question : problem.solution;
		std::ostringstream imploded_q;
		std::copy(q.begin(), q.end(), std::ostream_iterator<std::string>(imploded_q, " "));
		std::string question_joined = imploded_q.str();

		std::ostringstream imploded_s;
		std::copy(s.begin(), s.end(), std::ostream_iterator<std::string>(imploded_s, " "));
		std::string solution_joined = imploded_s.str();

		if (auto_language) {
			LAN language = utils::what_language(utils::to_utf16(solution_joined));
			set_os_lang(language);
			screen.set_language(language);
		}

		view::Screen::INPUT_STATE input_state;
		std::list<std::string> answer;

		try {
			update_statistic();
			screen.show_problem(problem);
			if (read_question)
				AudioRecord::play(question_joined);
			std::tie(input_state, answer) = screen.get_answer();
		} catch(const std::exception &e) {
			logging::Error() << e.what() << logging::endl;
			return 0;
		}

		if (input_state == view::Screen::INPUT_STATE::EXIT) {
			Parser::save_statistic(problems, problems_filename);
			return 0;
		}

		if (input_state == view::Screen::INPUT_STATE::SKIPPED) {
			to_solve.erase(std::remove(to_solve.begin(), to_solve.end(), solving_num), to_solve.end());
			update_statistic();
			screen.show_solution();
			screen.show_message("Skipped, press any key to continue");
			screen.wait_pressed_key();
			continue;
		}

		problem.was_attempt = true;
		an::Verification result = analyzer.check(problem, answer, analyze_flags);

		if (result.state == an::MARK::RIGHT) {
			--problem.repeat;
			if (problem.repeat == 0) {
				++solved_count;
				to_solve.erase(std::remove(to_solve.begin(), to_solve.end(), solving_num), to_solve.end());
			}
		} else {
			problem.repeat = REPEAT_TIMES;
			++problem.total_errors;
			++problem.errors;
			++errors_count;
		}

		if (play_solution)
			AudioRecord::play(solution_joined);

		while (true) {
			update_statistic();
			screen.show_result(result);
			screen.show_message(problem.repeat != 0
				? "Press space to play the question, F3 to skip it or another key to continue..."
				: "Press space to play the question or another key to continue...");

			int key = screen.wait_pressed_key();
			if (key == ' ') {
				AudioRecord::play(solution_joined);
				continue;
			} else if (key == view::FKEY::F3 && problem.repeat != 0) {
				problem.repeat = 0;
				++solved_count;
				to_solve.erase(std::remove(to_solve.begin(), to_solve.end(), solving_num), to_solve.end());
			} else
				break;
		}
	}

	Parser::save_statistic(problems, problems_filename);
	update_statistic();
	screen.show_message("All problems are solved, press eny key to exit");
	screen.wait_pressed_key();
	return 0;
}

