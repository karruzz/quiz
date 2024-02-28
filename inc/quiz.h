#ifndef QUIZ_H
#define QUIZ_H

#include <memory>
#include <random>
#include <vector>

#include <analyzer.h>
#include <options.h>
#include <problem.h>

using namespace analysis;

struct Statistics {
	int left_problems;
	int solved_problems;
	int errors;

	int problem_repeat_times;
	int problem_errors;
	int problem_total_errors;
};

class Quiz {
public:
	Quiz(Options& options);

	void initialize();

	void enable_topics(const std::list<std::string>& topics);
	void restart_quiz();

	std::shared_ptr<Problem> current_problem();
	std::vector<std::shared_ptr<Problem>>& quiz_problems();

	Verification apply_answer(const std::list<std::string>& answer);
	Statistics get_statistics();

private:
	Options _options;
	Analyzer _analyzer;

	std::vector<std::shared_ptr<Problem>> _all_problems;
	std::vector<std::shared_ptr<Problem>> _quiz_problems;

	int _errors_count = 0;
	int _solved_count = 0;

	void load_problems();
	void shuffle_problems();
};

#endif // QUIZ_H
