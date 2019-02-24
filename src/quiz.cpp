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
#define PLAY_QUESTION      "-q"
#define MIXED_MODE         "-m"
#define INVERT_MODE        "-i"
#define SHOW_STATISTIC     "-s"
#define REPEAT_ERRORS      "-r"
#define LANGUAGE_RECOGNIZE "-l"
#define ENTER_MODE         "-e"
#define NUMBERS            "-n"
#define CASE_UNSENSETIVE   "-c"
#define PUNCT_UNSENSETIVE  "-u"

const char * help_message =
	"-h	show this help\n" \
	"-p	play the solution\n" \
	"-q	play the question\n" \
	"-m	mixed mode, question and solution may be swapped\n" \
	"-i	invert questions and solutions, discard mixed mode (-m)\n" \
	"-s	show statistic\n" \
	"-r	include to quiz problems, which were with errors last time only\n" \
	"-l	input language auto-detect\n" \
	"-e	accept answer by enter key\n" \
	"-t	use topics\n" \
	"-c	case unsensitive\n" \
	"-u	punctuation unsensitive\n";

namespace {

namespace an = analysis;

typedef view::LANGUAGE LAN;

const int REPEAT_TIMES = 2;
const int TAB_SIZE = 4;

void play(const std::string& phrase)
{
	static char audio_play_cmd[100];
	if (phrase.empty()) return;
	sprintf(audio_play_cmd, "spd-say \"%s\"", phrase.c_str());
	system(audio_play_cmd);
}

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

	bool play_question = params.find(PLAY_QUESTION) != params.end();
	bool enter_accept_mode = params.find(ENTER_MODE) != params.end();
	bool invert_mode = params.find(INVERT_MODE) != params.end();
	bool mixed_mode = params.find(MIXED_MODE) != params.end() && !invert_mode;
	bool auto_language = params.find(LANGUAGE_RECOGNIZE) != params.end();
	bool repeat_errors_only = params.find(REPEAT_ERRORS) != params.end();
	bool show_statistic = params.find(SHOW_STATISTIC) != params.end();
	bool case_unsensitive = params.find(CASE_UNSENSETIVE) != params.end();
	bool punct_unsensitive = params.find(PUNCT_UNSENSETIVE) != params.end();

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
		if (invert_mode)
			problem.inverted = true;

		if (mixed_mode) {
			std::uniform_int_distribution<> distribution (0, 1);
			problem.inverted = distribution(generator) == 0;
			if (auto_language) {
				LAN language = problem.inverted ? LAN::RU : LAN::EN;
				set_os_lang(language);
				screen.set_language(language);
			}
		}

		view::Screen::INPUT_STATE input_state;
		std::list<std::string> answer;

		try {
			update_statistic();
			screen.show_problem(problem);
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
		int flags = an::Analyzer::NONE;
		if (case_unsensitive) flags |= an::Analyzer::CASE_UNSENSITIVE;
		if (punct_unsensitive) flags |= an::Analyzer::PUNCT_UNSENSITIVE;
		an::Verification result = analyzer.check(problem, answer, flags);

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

		std::string to_play;
		to_play = problem.solution.front();
		if (play_question) to_play = problem.question.front();
		if (!to_play.empty())
			play(to_play);

		while (true) {
			update_statistic();
			screen.show_result(result);
			screen.show_solution();
			screen.show_message(problem.repeat != 0
				? "Press space to play question, F3 to skip question or another key to continue"
				: "Press space to play question or another key to continue");

			int key = screen.wait_pressed_key();
			if (key == ' ') {
				play(to_play);
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
	screen.show_message("All problems solved, press eny key to exit");
	screen.wait_pressed_key();
	return 0;
}

