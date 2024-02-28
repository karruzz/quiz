#include <iterator>
#include <sstream>
#include <problem.h>

const std::list<std::string>& Problem::question() const
{
	return !inverted ? _question : _solution;
}

const std::list<std::string>& Problem::solution() const
{
	return !inverted ? _solution : _question;
}

static
std::string to_one_line(const std::list<std::string>& lines)
{
	std::ostringstream ostream;
	std::copy(lines.begin(), lines.end(), std::ostream_iterator<std::string>(ostream, " "));
	return ostream.str();
}

std::string Problem::question_str() const {
	return to_one_line(question());
}

std::string Problem::solution_str() const {
	return to_one_line(solution());
}

utils::Language Problem::question_lang() const {
	return !inverted ? _lang_question : _lang_solution;
}

utils::Language Problem::solution_lang() const {
	return !inverted ? _lang_solution : _lang_question;
}