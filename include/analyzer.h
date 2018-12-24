#ifndef ANALYZER_H
#define ANALYZER_H

#include <list>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "problem.h"

namespace analysis {

enum class MARK {
	RIGHT,
	INVALID_LINES_NUMBER,
	NOT_FULL_ANSWER,
	ERROR
};

struct Verification
{
	Problem problem;
	std::list<std::string> answer;
	std::list<std::string> solution;

	MARK state;
	// <line, <position_on_screen, string>>
	std::map<int, std::map<int, std::string>> errors;

	Verification(const Problem& p, const std::list<std::string>& a)
		: problem(p)
		, answer(a)
		, solution(!p.inverted ? p.solution : p.question)
		, state(MARK::RIGHT)
	{}

	Verification() = default;
	Verification(const Verification& v) = default;
	Verification& operator= (const Verification& v) = default;
};

class Analyzer
{
public:
	Verification check(const Problem& problem, const std::list<std::string>& answer);
};

} // namespace analyze

#endif // ANALYZER_H
