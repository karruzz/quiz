/*
 * analyzer.h
 *
 *  Created on: Dec 08, 2018
 *  Copyright © 2018-2081 Ilja Karasev <ilja.karasev@gmail.com>.
 *  All rights reserved.
 *     License: GNU GPL 3
 */

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

struct Token
{
	enum WHAT {
		UNDEF,
		WORD,
		SPACE,
		PUNCT
	};

	WHAT what;
	std::u16string str;
	size_t pos;

	bool operator<(const Token& l) {
		return str.compare(l.str);
	}
};

struct Error
{
	enum WHAT {
		ERROR_TOKEN,
		ERROR_SYMBOL,
		MISSED,
		REDUNDANT
	};

	WHAT what;
	std::u16string str;
	size_t pos; // position in string
};


struct Verification
{
	std::list<std::string> answer;
	std::list<std::string> solution;

	int state;
	// <line, Errors in line>
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
		NONE              = 0x0,
		CASE_UNSENSITIVE  = 0x1,
		PUNCT_UNSENSITIVE = 0x2,
		TOTAL_RECALL      = 0x4
	};

	Verification check(
		const Problem& problem, const std::list<std::string>& answer, int flags);

	static std::list<Token> split_to_tokens(const std::string& s);
};

} // namespace analyze

#endif // ANALYZER_H
