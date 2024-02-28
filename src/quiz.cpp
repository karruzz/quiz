#include <algorithm>

#include <parser.h>
#include <quiz.h>


Quiz::Quiz(Options &options)
	: _options(options) {

}

void Quiz::initialize() {
	load_problems();
}

void Quiz::load_problems() {
	_all_problems = Parser::load(_options);
}

void Quiz::shuffle_problems() {
	std::vector<int> problems_indexes;
	for (std::shared_ptr<Problem> p : _all_problems) {
		if (_options.get(Options::REPEAT_ERRORS_ONLY) && p->last_errors == 0)
			continue;

		_quiz_problems.push_back(p);
	}

	auto rd = std::random_device {};
	auto rng = std::default_random_engine { rd() };
	std::shuffle(_quiz_problems.begin(), _quiz_problems.end(), rng);
	_solved_count = 0;
	_errors_count = 0;
}


std::shared_ptr<Problem> Quiz::current_problem() {
	if (_quiz_problems.empty())
		return nullptr;

	return _quiz_problems.front();
}

std::vector<std::shared_ptr<Problem>>& Quiz::quiz_problems() {
	return _quiz_problems;
}

Verification Quiz::apply_answer(const std::list<std::string>& answer) {
	std::shared_ptr<Problem> problem = current_problem();

	Verification result = _analyzer.check(*problem, answer, _options);

	if (result.state == analysis::MARK::RIGHT) {
		problem->repeat--;
		if (problem->repeat == 0) {
			_solved_count++;
			_quiz_problems.erase(_quiz_problems.begin());
		}
	} else {
		static const int REPEAT_TIMES = 2;
		problem->repeat = REPEAT_TIMES;
		problem->total_errors++;
		problem->errors++;
		_errors_count++;

		auto rd = std::random_device {};
		auto rng = std::default_random_engine { rd() };
		std::uniform_int_distribution<> distribution(0, _quiz_problems.size() - 1);
		size_t new_position = distribution(rng);
        std::rotate(_quiz_problems.begin(), _quiz_problems.begin() + 1, _quiz_problems.begin() + new_position + 1);
	}

	return result;
}

Statistics Quiz::get_statistics() {
	std::shared_ptr<Problem> problem = current_problem();

	Statistics statistics = {
		static_cast<int>(_quiz_problems.size()),
		_solved_count,
		_errors_count,
		problem->repeat,
		problem->errors,
		problem->total_errors
	};

	return statistics;
}


