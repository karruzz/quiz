#ifndef PROBLEM_H
#define PROBLEM_H

#include <string>
#include <list>

/* problem example:
 *
 * % stdio    -> theme
 * # todo: check POSIX 2008
 * ^ tmpnam   -> tag
 * > create a name for a temporary file, show using headers   -> question
 * < stdio.h  --> solution
		 char *str = tmpnam(NULL);
		 if (str)
			 puts(str);
 *
 *
 *
 */


struct Problem {
	std::list<std::string> question;
	std::list<std::string> solution;
	size_t question_hash = 0;

	int repeat = 0;
	int total_errors = 0;
	int last_errors = 0;
	int errors = 0;

	bool was_attempt = false;
	bool inverted = false;

	Problem(const std::list<std::string> &q,
		const std::list<std::string> &s)
		: question(q)
		, solution(s)
		, question_hash(0)
		, repeat(1)
		, total_errors(0)
		, last_errors(0)
		, errors(0)
		, was_attempt(false)
		, inverted(false)
	{}

	Problem() = default;
	Problem(const Problem& p) = default;
	Problem& operator= (const Problem& p) = default;

	bool operator< (const Problem &p) const {
		return question_hash < p.question_hash;
	}
};

#endif // PROBLEM_H
