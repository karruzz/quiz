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

#include "viewer.h"
#include "ncurces_screen.h"
#include "problem.h"
#include "parser.h"
#include "analyzer.h"

#define SHOW_HELP          "-h"
#define PLAY_SOLUTION      "-p"
#define PLAY_QUESTION      "-q"
#define MIXED_MODE         "-m"
#define INVERT_MODE        "-i"
#define SHOW_STATISTIC     "-s"
#define REPEAT_ERRORS      "-r"
#define LANGUAGE_RECOGNIZE "-l"
#define ENTER_MODE         "-e"
#define NUMBERS            "-n"

const char * help_message =
	"-h	show this help\n" \
	"-p	play the solution\n" \
	"-q	play the question\n" \
	"-m	mixed mode, question and solution may be swapped\n" \
	"-i	invert questions and solution, discards mixed mode (-m)\n" \
	"-s	show statistic\n" \
	"-r	repeat only problems, which were with errors last time\n" \
	"-l	answer language auto-detect\n" \
	"-e	accept answer by enter key\n" \
	"-t	use topics\n";

// todo:
// -c - case insensetive
// log
// variants for right answer
// grammar checking

namespace {

typedef view::LANGUAGE LAN;

const int REPEAT_TIMES = 2;

void play(const std::string& phrase)
{
	static char audio_play_cmd[100];
	if (phrase.empty()) return;
	sprintf(audio_play_cmd, "spd-say \"%s\"", phrase.c_str());
	system(audio_play_cmd);
}

void set_system_lang(LAN language)
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
		std::cerr << "argv is empty - no input file name" << std::endl;
		return 1;
	}

	std::string problems_filename(argv[1]);

	// split params on keys and values
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
			std::cerr << "arguments without key" << std::endl;
			return 1;
		}

		params.at(key).push_back(param);
	}

	if (problems_filename == "-h" || params.find(SHOW_HELP) != params.end()) {
		std::cout << help_message;
		return 0;
	}

	bool play_solution = params.find(PLAY_SOLUTION) != params.end();
	bool play_question = params.find(PLAY_QUESTION) != params.end();
	bool enter_accept_mode = params.find(ENTER_MODE) != params.end();
	bool invert_mode = params.find(INVERT_MODE) != params.end();
	bool mixed_mode = params.find(MIXED_MODE) != params.end() && !invert_mode;
	bool auto_language = params.find(LANGUAGE_RECOGNIZE) != params.end();

	std::vector<Problem> problems = Parser::load(problems_filename, params);

	if (params.find(SHOW_STATISTIC) != params.end()) {
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
		}
		return 0;
	}

	setlocale(LC_ALL, "");

	std::random_device rd;  // used to obtain a seed for the random number engine
	std::mt19937 generator(rd()); // standard mersenne_twister_engine seeded with rd()

	std::vector<int> to_solve;
	int solving_num = -1, previous_solving_num = -1;

	bool repeat_errors_only = params.find(REPEAT_ERRORS) != params.end();
	for (size_t i = 0; i < problems.size(); ++i) {
		if (repeat_errors_only && problems[i].last_errors == 0)
			continue;

		to_solve.push_back(static_cast<int>(i));
	}

	view::ncurses::NScreen screen(enter_accept_mode);
	int test_total_errors = 0, problems_solved = 0;

	auto update_statistic = [&]() {
		view::Statistic statistic = {
			static_cast<int>(to_solve.size()),
			problems_solved,
			test_total_errors,
			problems.at(solving_num).repeat,
			problems.at(solving_num).errors,
			problems.at(solving_num).total_errors
		};
		screen.update_statistic(statistic);
	};

	analysis::EqualAnalyzer analyzer;
	while (to_solve.size() != 0) {
		do {
			std::uniform_int_distribution<> distribution (0, to_solve.size() - 1);
			solving_num = to_solve[distribution(generator)];
		} while (to_solve.size() > 1 && solving_num == previous_solving_num);
		previous_solving_num = solving_num;

		Problem& problem = problems[solving_num];
		if (invert_mode)
			problem.inverted = true;

		bool qa_swapped = false;
		if (mixed_mode) {
			std::uniform_int_distribution<> distribution (0, 1);
			if (distribution(generator) % 2 == 0) {
				problem.inverted = true;
				qa_swapped = true;
			}
		}

		view::Screen::INPUT_STATE input_state;
		std::list<std::string> answer;

		try {
			update_statistic();
			if (auto_language) {
				LAN language = qa_swapped ? LAN::EN : LAN::RU;
				set_system_lang(language);
				screen.set_language(language);
			}
			screen.show_problem(problem);
			std::tie(input_state, answer) = screen.get_answer();
		} catch(const std::exception &e) {
			std::cerr << e.what() << std::endl;
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
		analysis::Verification result = analyzer.check(problem, answer);

		if (result.state == analysis::MARK::RIGHT) {
			--problem.repeat;
			if (problem.repeat == 0) {
				++problems_solved;
				to_solve.erase(std::remove(to_solve.begin(), to_solve.end(), solving_num), to_solve.end());
			}
		} else {
			problem.repeat = REPEAT_TIMES;
			++problem.total_errors;
			++problem.errors;
			++test_total_errors;
		}

		update_statistic();
		screen.show_result(result);
		screen.show_solution();
		screen.show_message("Press space to play question or another key to continue");

		std::string to_play;
		if (play_question) to_play = problem.question.front();
		if (play_solution) to_play = problem.solution.front();
		if (!to_play.empty())
			play(to_play);

		while (screen.wait_pressed_key() == ' ')
			play(to_play);
	}

	Parser::save_statistic(problems, problems_filename);
	update_statistic();
	screen.show_message("All problems solved, press eny key to exit");
	screen.wait_pressed_key();
	return 0;
}

