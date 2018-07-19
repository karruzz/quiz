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
 * < stdio.h  --> answer
		 char *str = tmpnam(NULL);
		 if (str)
			 puts(str);
 *
 *
 *
 */


struct Problem {
	std::list<std::string> question;
	std::list<std::string> answer;
	size_t question_hash;

	int repeat;
	int total_errors;
	int last_errors;
	int errors;

	bool was_attempt_any_time_before; // to answer
	bool was_attempt_this_time;

	Problem(const std::list<std::string> &q,
		const std::list<std::string> &a,
		int repeat)
		: question(q)
		, answer(a)
		, question_hash(0)
		, repeat(repeat)
		, total_errors(0)
		, last_errors(0)
		, errors(0)
		, was_attempt_any_time_before(false)
		, was_attempt_this_time(false)
	{}

	Problem(const Problem& p) = default;
	Problem& operator= (const Problem& p) = default;

	bool operator< (const Problem &p) const {
		return question_hash < p.question_hash;
	}
};

#endif // PROBLEM_H
