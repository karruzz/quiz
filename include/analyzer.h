#ifndef ANALYZER_H
#define ANALYZER_H

#include <list>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "problem.h"

namespace analysis {

enum MARK {
	RIGHT                = 0x0,
	INVALID_LINES_NUMBER = 0x1,
	NOT_FULL_ANSWER      = 0x2,
	REDUNDANT_ANSWER     = 0x4,
	ERROR                = 0x8
};

struct Error
{
	enum WHAT {
		ERROR_LEXEM,
		ERROR_SYMBOL
	};

	WHAT what;
	std::u16string str;
	size_t pos;
};


struct Verification
{
	std::list<std::string> answer;
	std::list<std::string> solution;

	int state;
	// <line, <position_on_screen, string>>
	std::map<int, std::list<Error>> errors;

	Verification(const Problem& p, const std::list<std::string>& a)
		: answer(a)
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
	enum OPTIONS {
		NONE = 0,
		CASE_INSENSITIVE
	};

	Verification check(
		const Problem& problem, const std::list<std::string>& answer, Analyzer::OPTIONS flags);
};

} // namespace analyze

#endif // ANALYZER_H
