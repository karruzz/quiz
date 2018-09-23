// learn_lan.cpp : Defines the entry point for the console application.
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

#include "viewer.h"
#include "ncurces_screen.h"
#include "problem.h"
#include "parser.h"

#define SHOW_HELP        "-h"  // show help
#define NOT_SHOW_ANSWERS "-a"  // not show right answer when was mistake
#define MIXED_MODE       "-m"  // question-answer mode mixed with answer-question mode
#define SHOW_STATISTIC   "-s"  // show statistic
#define REPEAT_ERRORS    "-r"  // repeat only problems, which were with errors last time

// todo:
// -c - case insensetive
// -d - recognize Deutch letters
// copy paste
// screen update on resize
// all list to vector
// -h help
// -l auto language
// update statistic line for current question after answer instantly

int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cerr << "argv is empty - no input file name" << std::endl;
		return 1;
	}

	std::string test_file_name(argv[1]);

	bool show_right_answer = true;
	bool mixed_mode = false;

	// save all params beside first
	std::vector<std::string> params;
	for (int i = 2; i < argc; i++) {
		params.push_back(argv[i]);
	}

	// split params on keys and values
	std::map<std::string, std::vector<std::string> > paramsMap;
	std::vector<std::string>::iterator it_params = params.begin();
	std::string key;
	for(; it_params != params.end(); ++it_params) {
		if ((*it_params)[0] == '-') {
			key = *it_params;
			paramsMap[key] = std::vector<std::string>();
			continue;
		}

		if (key.empty()) {
			std::cerr << "arguments without key" << std::endl;
			return 1;
		}

		paramsMap[key].push_back(*it_params);
	}

	if (test_file_name == "-h" || paramsMap.find(SHOW_HELP) != paramsMap.end()) {
		std::cout << "-h	show this help" << std::endl;
		std::cout << "-a	not show answers" << std::endl;
		std::cout << "-m	question-answer mode mixed with answer-question mode" << std::endl;
		std::cout << "-s	show statistic" << std::endl;
		std::cout << "-r	repeat only problems, which were with errors last time" << std::endl;
		return 0;
	}

	if (paramsMap.find(NOT_SHOW_ANSWERS) != paramsMap.end())
		show_right_answer = false;

	if (paramsMap.find(MIXED_MODE) != paramsMap.end())
		mixed_mode = true;

	std::vector<Problem> problems = Parser::load(test_file_name, paramsMap);

	if (paramsMap.find(SHOW_STATISTIC) != paramsMap.end()) {
		for (std::vector<Problem>::iterator it = problems.begin(); it != problems.end(); ++it) {
			std::cout << "? ";
			for (auto it_q = it->question.begin(); it_q != it->question.end(); ++it_q)
				std::cout << *it_q << std::endl;

			std::cout << "> ";
#ifdef DEBUG
			std::cout << "hash: " << it->question_hash << "; ";
#endif
			std::cout << "total_errors: " << it->total_errors << "; " ;
			std::cout << "last_errors: " << it->last_errors << "; ";
			std::cout << "was attempts: " << it->was_attempt_any_time_before << std::endl << std::endl;
		}
		return 0;
	}

	setlocale(LC_ALL, "");

	std::random_device rd;  //Will be used to obtain a seed for the random number engine
	std::mt19937 generator(rd()); //Standard mersenne_twister_engine seeded with rd()

	std::vector<int> to_solve;
	std::vector<int>::size_type solving_num;

	bool repeat_errors_only = paramsMap.find(REPEAT_ERRORS) != paramsMap.end();
	for (size_t i = 0; i < problems.size(); ++i) {
		if (repeat_errors_only
			&& problems[i].last_errors == 0
			&& problems[i].was_attempt_any_time_before)
		{
			continue;
		}
		to_solve.push_back(static_cast<int>(i));
	}

	view::ncurses::NScreen screen(show_right_answer);
	int test_total_errors = 0, problems_total = to_solve.size(), problems_solved = 0;

	while (to_solve.size() != 0) {
		std::uniform_int_distribution<> distribution (0, to_solve.size() - 1);
		solving_num = to_solve[distribution(generator)];

		std::list<std::string> question = problems[solving_num].question;
		std::list<std::string> answer = problems[solving_num].answer;
		if (mixed_mode) {
			std::vector<int>::size_type mixed_random = distribution(generator);
			if (mixed_random % 2 == 0) question.swap(answer);
		}

		view::Statistic statistic = {
			problems_total,
			problems_solved,
			test_total_errors,
			problems[solving_num].repeat,
			problems[solving_num].errors,
			problems[solving_num].total_errors
		};

		view::Screen::INPUT_STATE answer_state;
		std::list<std::string> user_answer;
		std::map<int, int> errors;

		try {
			screen.update_statistic(statistic);
			screen.show_question(question);
			std::tie(answer_state, user_answer) = screen.get_answer();
		} catch(const std::exception &e) {
			std::cerr << e.what() << std::endl;
			return 0;
		}

		user_answer.remove("");

		if (answer_state == view::Screen::INPUT_STATE::EXIT) {
			Parser::save_statistic(problems, test_file_name);
			return 0;
		}

		if (answer_state == view::Screen::INPUT_STATE::SKIPPED) {
			to_solve.erase(std::remove(to_solve.begin(), to_solve.end(), solving_num), to_solve.end());
			screen.show_result(view::Screen::CHECK_STATE::SKIPPED, answer, errors);
			continue;
		}

		problems[solving_num].was_attempt_any_time_before = true;
		problems[solving_num].was_attempt_this_time = true;

		if (user_answer.size() != answer.size()) {
			++test_total_errors;
			++problems[solving_num].total_errors;
			++problems[solving_num].errors;
			screen.show_result(view::Screen::CHECK_STATE::LINES_NUMBER_ERROR, answer, errors);
			continue;
		}

		errors = Parser::check_answer(user_answer, answer);
		view::Screen::CHECK_STATE check_state;
		if (errors.size() == 0) {
			check_state = view::Screen::CHECK_STATE::RIGHT;
			--problems[solving_num].repeat;
			if (problems[solving_num].repeat == 0) {
				++problems_solved;
				to_solve.erase(std::remove(to_solve.begin(), to_solve.end(), solving_num), to_solve.end());
			}
		} else {
			check_state = view::Screen::CHECK_STATE::INVALID;
			if (problems[solving_num].repeat < 2) ++problems[solving_num].repeat;
			++problems[solving_num].total_errors;
			++problems[solving_num].errors;
			++test_total_errors;
		}

		screen.show_result(check_state, answer, errors);
	}

	Parser::save_statistic(problems, test_file_name);

	screen.show_result(view::Screen::CHECK_STATE::ALL_SOLVED,
		std::list<std::string>(), std::map<int, int>());
	return 0;
}

