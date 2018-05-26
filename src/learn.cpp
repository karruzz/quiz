// learn_lan.cpp : Defines the entry point for the console application.
//

#include <iostream>

#include <vector>
#include <map>

#include <iterator>
#include <string>
#include <sstream>
#include <random>
#include <algorithm>
#include <memory>
#include <ctime>
#include <cctype>
#include <tuple>

#include "viewer.h"
#include "ncurces_screen.h"
#include "problem.h"
#include "parser.h"

#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>

#define NOT_SHOW_ANSWERS "-a"
#define MIXED_MODE       "-m"  // question-answer mixed with answer-question
#define SHOW_TAGS        "-s"  // show all tags
#define LEARN_LAST_ERROR "-e"

int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cerr << "argv is empty - no input file name" << std::endl;
		return 1;
	}

	std::string test_file_name(argv[1]);

	bool show_right_answer = true;
	bool mixed_mode = false;

	//save all params, besides first
	std::vector<std::string> params;
	for (int i = 2; i < argc; i++) {
		params.push_back(argv[i]);
	}

	// split param on keys and values
	std::map<std::string, std::vector<std::string> > paramsMap;
	std::vector<std::string>::iterator it_params = params.begin();
	std::string key;
	for(; it_params != params.end(); it_params++) {
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

	if (paramsMap.find(NOT_SHOW_ANSWERS) != paramsMap.end())
		show_right_answer = false;

	if (paramsMap.find(MIXED_MODE) != paramsMap.end())
		mixed_mode = true;

	std::vector<Problem> problems = Parser::load(test_file_name, paramsMap);

	if (paramsMap.find(SHOW_TAGS) != paramsMap.end()) {
		std::vector<std::string> tags;
		for (std::vector<Problem>::iterator it = problems.begin(); it != problems.end(); ++it) {
			std::vector<std::string> strs;
			boost::split(strs, it->tag, boost::is_any_of("\t, "));
			tags.insert(tags.end(), strs.begin(), strs.end());
		}

		std::sort(tags.begin(), tags.end());
		for (std::vector<std::string>::iterator it = tags.begin(); it != tags.end(); ++it)
			std::cout << *it << std::endl;
		return 0;
	}

	setlocale(LC_ALL, "");

	std::random_device rd;  //Will be used to obtain a seed for the random number engine
	std::mt19937 generator(rd()); //Standard mersenne_twister_engine seeded with rd()

	std::vector<int> to_solve;
	std::vector<int>::size_type solving_num;

	bool learn_last_error = paramsMap.find(LEARN_LAST_ERROR) != paramsMap.end();
	for (size_t i = 0; i < problems.size(); ++i) {
		if (learn_last_error && problems[i].last_errors == 0) continue;
		to_solve.push_back(static_cast<int>(i));
	}

	view::NcursesScreen screen(show_right_answer);
	int total_mistakes = 0, problems_total = to_solve.size(), problems_left;

	while ((problems_left = to_solve.size()) != 0) {
		std::uniform_int_distribution<> distribution (0, problems_left - 1);
		solving_num = to_solve[distribution(generator)];

		std::list<std::string> question = problems[solving_num].question;
		std::list<std::string> answer = problems[solving_num].answer;
		if (mixed_mode) {
			std::vector<int>::size_type mixed_random = distribution(generator);
			if (mixed_random % 2 == 0) question.swap(answer);
		}

		view::Statistic statistic = {
			problems_total,
			problems_left,
			total_mistakes,
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

		if (answer_state == view::Screen::INPUT_STATE::EXIT) return 0;
		if (answer_state == view::Screen::INPUT_STATE::SKIPPED) {
			to_solve.erase(std::remove(to_solve.begin(), to_solve.end(), solving_num), to_solve.end());
			screen.show_result(view::Screen::CHECK_STATE::SKIPPED, answer, errors);
			continue;
		}

		if (user_answer.size() != answer.size()) {
			++total_mistakes;
			screen.show_result(view::Screen::CHECK_STATE::LINES_NUMBER_ERROR, answer, errors);
			continue;
		}

		errors = Parser::check_answer(user_answer, answer);
		view::Screen::CHECK_STATE check_state;
		if (errors.size() == 0) {
			check_state = view::Screen::CHECK_STATE::RIGHT;
			--problems[solving_num].repeat;
			if (problems[solving_num].repeat == 0)
				to_solve.erase(std::remove(to_solve.begin(), to_solve.end(), solving_num), to_solve.end());
		} else {
			check_state = view::Screen::CHECK_STATE::INVALID;
			++problems[solving_num].repeat;
			++problems[solving_num].total_errors;
			++problems[solving_num].errors;
			++total_mistakes;
		}

		screen.show_result(check_state, answer, errors);
	}

	Parser::save_statistic(problems, test_file_name);

	screen.show_result(view::Screen::CHECK_STATE::ALL_SOLVED,
		std::list<std::string>(), std::map<int, int>());
	return 0;
}

