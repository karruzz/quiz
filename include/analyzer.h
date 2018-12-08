#ifndef ANALYZER_H
#define ANALYZER_H

#include <map>
#include <string>
#include <list>
#include <tuple>

#include "problem.h"

namespace analysis {

enum class MARK {
	RIGHT,
	INVALID_LINES_NUMBER,
	ERROR
};

struct Verification
{
	Problem problem;
	std::list<std::string> answer;

	MARK state;
	std::map<int, int> errors;

	Verification(const Problem& p, const std::list<std::string>& a)
		: problem(p)
		, answer(a)
	{}

	Verification() = default;
	Verification(const Verification& v) = default;
	Verification& operator= (const Verification& v) = default;
};

class BaseAnalyzer
{
public:
	virtual Verification check(const Problem& problem, const std::list<std::string>& answer);
};

class EqualAnalyzer : BaseAnalyzer
{
public:
	virtual Verification check(const Problem& problem, const std::list<std::string>& answer);
};

} // namespace analyze

#endif
