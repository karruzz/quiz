#include <analyzer.h>
#include <algorithm>
#include <vector>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace analysis {

Verification BaseAnalyzer::check(
	const Problem& problem, const std::list<std::string>& answer)
{
	Verification v(problem, answer);
	v.answer.remove("");
	if(v.solution.size() != v.answer.size())
		v.state = MARK::INVALID_LINES_NUMBER;

	v.errors = std::map<int, std::map<int, int>>();
	return v;
}

Verification EqualAnalyzer::check(
	const Problem& problem, const std::list<std::string>& answer)
{
	auto v = BaseAnalyzer::check(problem, answer);
	if (v.state != MARK::RIGHT) return v;

	int line_num = 0;
	auto answr_it = v.answer.cbegin();
	auto solut_it = v.solution.cbegin();
	for (; answr_it != v.answer.cend() && solut_it != v.solution.cend()
		 ; ++answr_it, ++solut_it, ++line_num)
	{
		for (size_t i = 0; i < solut_it->size(); i++) {
			if (i >= answr_it->size() || solut_it->at(i) != answr_it->at(i)) {
				v.errors[line_num].insert(std::pair<int, int>(i, 1));
				break;
			}
		}

		// redundant symbols at the end of the answer
		if (v.errors.find(line_num) == v.errors.end() && answr_it->size() > solut_it->size())
			v.errors[line_num].insert(std::pair<int, int>(solut_it->size(), 1));
	}

	if (v.errors.size() != 0) v.state = MARK::ERROR;
	return v;
}

static
std::vector<std::string> split_to_lexemes(const std::string& s)
{
	std::vector<std::string> tokens;
	boost::split( tokens, s, boost::is_any_of(" ") );
	return tokens;
}

int is_space_not_tab(int c) {
	return static_cast<int>(std::isspace(c) && c != '\t');
}

void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(),
			std::not1(std::ptr_fun<int, int>(is_space_not_tab))));
}

void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(),
			std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

void trim_spaces(std::string &s) {
	ltrim(s);
	rtrim(s);
}

static
void remove_duplicate_space(std::string& src)
{
	static auto adjacent_spaces = [](char lhs, char rhs) { return (rhs == ' ') && (lhs == ' '); };
	src.erase(std::unique(src.begin(), src.end(), adjacent_spaces), src.end());
}

Verification GrammarAnalyzer::check(
		const Problem& problem, const std::list<std::string>& answer)
{
	auto v = BaseAnalyzer::check(problem, answer);
	if (v.state != MARK::RIGHT) return v;
	
	std::for_each(v.answer.begin(), v.answer.end(), trim_spaces);
	std::for_each(v.answer.begin(), v.answer.end(), remove_duplicate_space);

	auto answr_it = v.answer.begin();
	auto solut_it = v.solution.cbegin();
	int line_num = 0;
	for (; answr_it != v.answer.end() && solut_it != v.solution.cend()
		 ; ++answr_it, ++solut_it, ++line_num)
	{
		std::string& answr_line = *answr_it;
		std::string solut_line = *solut_it;

		auto answr_lexems = split_to_lexemes(answr_line);
		auto solut_lexems = split_to_lexemes(solut_line);

		int position = 0;
		auto answr_lexems_it = answr_lexems.cbegin();
		auto solut_lexems_it = solut_lexems.cbegin();
		for (; answr_lexems_it != answr_lexems.cend() && solut_lexems_it != solut_lexems.cend()
			 ; position+=answr_lexems_it->length() + 1, ++answr_lexems_it, ++solut_lexems_it)
		{
			if (*answr_lexems_it != *solut_lexems_it) {
				v.errors[line_num].insert(std::pair<int, int>(position, answr_lexems_it->size()));
				break;
			}
		}

		if (v.errors.find(line_num) == v.errors.end()) {
			// not full answer
			if (solut_lexems_it != solut_lexems.cend()) {
				int pos = answr_line.size();
				answr_line.append("...");
				v.errors[line_num].insert(std::pair<int, int>(pos, 3));
				v.state = MARK::NOT_FULL_ANSWER;
			}
			// redundant answer
			if (answr_lexems_it != answr_lexems.cend()) {
				for (; answr_lexems_it != answr_lexems.cend()
					 ; position+=answr_lexems_it->size() + 1, ++answr_lexems_it) {
					v.errors[line_num].insert(std::pair<int, int>(position, answr_lexems_it->size()));
				}
			}
		}
	}

	if (v.state == MARK::RIGHT && v.errors.size() != 0)
		v.state = MARK::ERROR;
	return v;
}

} // namespace analyze

