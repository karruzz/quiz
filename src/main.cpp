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

#include <analyzer.h>
#include <log.h>
#include <ncurces_screen.h>
#include <options.h>
#include <problem.h>
#include <parser.h>
#include <viewer.h>

// todo: voice refactor
//       log debug to window_debug, which could be hidden

namespace {

namespace an = analysis;

const int REPEAT_TIMES = 2;
const int TAB_SIZE = 4;
const int ERROR_CODE = 1;

static Options options;

void set_os_lang(utils::Language language)
{
	if (language == utils::Language::RU)
		system("setxkbmap -layout ru,us -option grp:alt_shift_toggle");
	else if (language == utils::Language::EN)
		system("setxkbmap -layout us,ru -option grp:alt_shift_toggle");
}

} // namespace

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	if (!options.parse_arguments(argc, argv))
		return ERROR_CODE;

	if (options.help()) {
		logging::Message() << cmd::HELP_MESSAGE;
		return 0;
	}

	std::vector<std::shared_ptr<Problem>> problems = Parser::load(options);

	if (options.get(Options::SHOW_STATISTICS)) {
		logging::Message msg;
		for (std::shared_ptr<Problem> p: problems) {
			msg << "? ";
			for (const std::string& q: p->question())
				msg << q << logging::endl;

			msg << "> ";
#ifdef DEBUG
			msg << "hash: " << p->question_hash << "; ";
#endif
			msg << "total errors: " << p->total_errors << "; " ;
			msg << "last errors: " << p->last_errors << ";" << logging::endl;
		}
		return 0;
	}

	std::random_device rd;  // used to obtain a seed for the random number engine
	std::mt19937 generator(rd()); // standard mersenne_twister_engine seeded with rd()

	std::vector<int> to_solve;
	for (size_t i = 0; i < problems.size(); ++i) {
		if (options.get(Options::REPEAT_ERRORS_ONLY)
		 && problems[i]->last_errors == 0)
			continue;

		to_solve.push_back(static_cast<int>(i));
	}

	view::ncurses::NScreen screen(options.get(Options::ACCEPT_BY_ENTER), TAB_SIZE);
	int errors_count = 0, solved_count = 0, solving_num = -1, previous_solving_num = -1;

	auto update_statistic = [&]() {
		Statistics statistics = {
			static_cast<int>(to_solve.size()),
			solved_count,
			errors_count,
			problems.at(solving_num)->repeat,
			problems.at(solving_num)->errors,
			problems.at(solving_num)->total_errors
		};
		screen.update_statistic(statistics);
	};

	an::Analyzer analyzer;
	while (to_solve.size() != 0) {
		do {
			std::uniform_int_distribution<> distribution (0, to_solve.size() - 1);
			solving_num = to_solve[distribution(generator)];
		} while (to_solve.size() > 1 && solving_num == previous_solving_num);
		previous_solving_num = solving_num;

		std::shared_ptr<Problem> problem = problems[solving_num];
		problem->inverted = options.get(Options::QS_INVERTED);
		problem->not_show_question = options.get(Options::HIDE_QUESTION);

		if (options.get(Options::QS_MIXED)) {
			static std::uniform_int_distribution<> distribution (0, 1);
			problem->inverted = distribution(generator) == 0;
		}

		auto q = problem->question();
		auto s = problem->solution();

		auto ql = problem->question_lang();
		auto sl = problem->solution_lang();

		std::ostringstream ostream_q;
		std::copy(q.begin(), q.end(), std::ostream_iterator<std::string>(ostream_q, " "));
		std::string question = ostream_q.str();

		std::ostringstream ostream_s;
		std::copy(s.begin(), s.end(), std::ostream_iterator<std::string>(ostream_s, " "));
		std::string solution = ostream_s.str();

		if (options.get(Options::AUTO_LANGUAGE)) {
			utils::Language language = utils::what_language(utils::to_utf16(solution));
			set_os_lang(language);
			screen.set_language(language);
		}

		view::Screen::INPUT_STATE input_state;
		std::list<std::string> answer;

		try {
			update_statistic();
			screen.show_problem(*problem);
			if (options.get(Options::READ_QUESTION))
				AudioRecord::play(question, ql);
			std::tie(input_state, answer) = screen.get_answer();
		} catch(const std::exception &e) {
			logging::Error() << e.what() << logging::endl;
			return 0;
		}

		if (input_state == view::Screen::INPUT_STATE::EXIT) {
			Parser::save_statistic(problems, options.filename());
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

		problem->was_attempt = true;
		an::Verification result = analyzer.check(*problem, answer, options);

		if (result.state == an::MARK::RIGHT) {
			--problem->repeat;
			if (problem->repeat == 0) {
				++solved_count;
				to_solve.erase(std::remove(to_solve.begin(), to_solve.end(), solving_num), to_solve.end());
			}
		} else {
			problem->repeat = REPEAT_TIMES;
			++problem->total_errors;
			++problem->errors;
			++errors_count;
		}

		if (options.get(Options::PLAY_SOLUTION))
			AudioRecord::play(solution, sl);

		while (true) {
			update_statistic();
			screen.show_result(result);
			screen.show_message(problem->repeat != 0
				? "Press space to play the question, F3 to skip it or another key to continue..."
				: "Press space to play the question or another key to continue...");

			int key = screen.wait_pressed_key();
			if (key == ' ') {
				AudioRecord::play(solution, sl);
				continue;
			} else if (key == view::FKEY::F3 && problem->repeat != 0) {
				problem->repeat = 0;
				++solved_count;
				to_solve.erase(std::remove(to_solve.begin(), to_solve.end(), solving_num), to_solve.end());
			} else
				break;
		}
	}

	Parser::save_statistic(problems, options.filename());
	update_statistic();
	screen.show_message("All problems are solved, press eny key to exit");
	screen.wait_pressed_key();
	return 0;
}

