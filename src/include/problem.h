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
	std::string tag;
	std::list<std::string> question;
	std::list<std::string> answer;
	int repeat;
	int total_errors;
	int last_errors;
	int errors;

	Problem(std::string t, std::list<std::string> q, std::list<std::string> a, int repeat)
		: tag(t), question(q), answer(a), repeat(repeat) {};

	Problem(const Problem& p)
		: tag(p.tag), question(p.question), answer(p.answer), repeat(p.repeat) {};

	Problem& operator= (const Problem& p) {
		tag = p.tag;
		question = p.question;
		answer = p.answer;
		repeat = p.repeat;
		return *this;
	}

	bool operator< (const Problem &p) const {
		return tag < p.tag;
	}
};

#endif // PROBLEM_H
