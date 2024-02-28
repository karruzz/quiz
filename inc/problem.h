/*
 * problem.h
 *
 *  Created on: May 19, 2018
 *  Copyright © 2018-2081 Ilja Karasev <ilja.karasev@gmail.com>.
 *  All rights reserved.
 *     License: GNU GPL 3
 */

#ifndef PROBLEM_H
#define PROBLEM_H

#include <string>
#include <list>
#include <utils.h>

/* problem example:
 *
 * % stdio    -> theme
 * # todo: check POSIX 2008 -> comment
 * ^ tmpnam   -> tag (deprecated)
 * > create a name for a temporary file, show using headers   -> question
 * < stdio.h  --> solution
		 char *str = tmpnam(NULL);
		 if (str)
			 puts(str);
 *
 *
 *
 */


class Problem {
public:
	size_t question_hash = 0;

	int repeat = 1;
	int total_errors = 0;
	int last_errors = 0;
	int errors = 0;

	bool was_attempt = false;
	bool inverted = false;
	bool not_show_question = false;

	Problem(
		const std::list<std::string> &q,
		const std::list<std::string> &s,
		utils::Language lang_question,
		utils::Language lang_solution)
		: _question(q)
		, _solution(s)
		, _lang_question(lang_question)
		, _lang_solution(lang_solution)
	{}

	Problem() = default;
	Problem(const Problem& p) = default;
	Problem& operator= (const Problem& p) = default;

	bool operator< (const Problem &p) const {
		return question_hash < p.question_hash;
	}

	const std::list<std::string>& question() const;
	const std::list<std::string>& solution() const;

	std::string question_str() const;
	std::string solution_str() const;

	utils::Language question_lang() const;
	utils::Language solution_lang() const;

private:
	std::list<std::string> _question;
	std::list<std::string> _solution;

	utils::Language _lang_question;
	utils::Language _lang_solution;
};

#endif // PROBLEM_H
