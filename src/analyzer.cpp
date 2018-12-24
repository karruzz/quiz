#include <algorithm>
#include <set>
#include <vector>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <analyzer.h>
#include <utils.h>

namespace analysis {

namespace {

struct Lexem
{
	enum TYPE {
		UNDEF = 0,
		WORD,
		SPACE,
		PUNCT
	};

	TYPE what;
	std::string str;
	size_t pos;
	size_t length;
};

std::vector<Lexem> split_to_lexemes(const std::string& s)
{
	static const std::set<std::string> space_lexems = { " ", "\t" };
	static const std::set<std::string> punctuate_lexems
		= { ",", ".", "?", "!", "-", ":", ";", "_", "(", ")", "[", "]", "<", ">", "{", "}", "+", "-", "=", "*", "/" };
	std::vector<std::string> tokens = utf8_split(s);
	std::vector<Lexem> lexemes;
	std::string lexem;
	size_t position = 0;
	Lexem::TYPE prev_lexem_type = Lexem::TYPE::UNDEF, lexem_type = Lexem::TYPE::UNDEF;

	auto try_to_flush_lexem = [&]() {
		if (prev_lexem_type != Lexem::TYPE::UNDEF && prev_lexem_type != lexem_type) {
			size_t length = utf8_length(lexem);
			lexemes.push_back({ lexem_type, lexem, position, length });
			position += length;
			lexem.clear();
		}
	};

	for (const std::string& t: tokens) {
		if (space_lexems.find(t) != space_lexems.end()) {
			lexem_type = Lexem::SPACE;
			try_to_flush_lexem();
			lexem = t;
		} else if (punctuate_lexems.find(t) != punctuate_lexems.end()) {
			lexem_type = Lexem::PUNCT;
			try_to_flush_lexem();
			lexem = t;
		} else {
			lexem_type = Lexem::WORD;
			try_to_flush_lexem();
			lexem.append(t);
		}

		prev_lexem_type = lexem_type;
	}

	// final lexem
	if (!lexem.empty())
		lexemes.push_back({ lexem_type, lexem, position, utf8_length(lexem) });

	return lexemes;
}

} // namespace

Verification Analyzer::check(
		const Problem& problem, const std::list<std::string>& answer)
{
	Verification v(problem, answer);
	v.answer.remove("");
	if(v.solution.size() != v.answer.size()) {
		v.state = MARK::INVALID_LINES_NUMBER;
		return v;
	}

	std::for_each(v.answer.begin(), v.answer.end(), trim_spaces);
	std::for_each(v.answer.begin(), v.answer.end(), remove_duplicate_spaces);

	int line_num = 0;
	auto answr_line_it = v.answer.cbegin();
	auto solut_line_it = v.solution.cbegin();
	for (; answr_line_it != v.answer.cend() && solut_line_it != v.solution.cend()
		 ; ++answr_line_it, ++solut_line_it, ++line_num)
	{
		auto answr_lexems = split_to_lexemes(*answr_line_it);
		auto solut_lexems = split_to_lexemes(*solut_line_it);

		auto answr_lexems_it = answr_lexems.cbegin();
		auto solut_lexems_it = solut_lexems.cbegin();
		for (; answr_lexems_it != answr_lexems.cend() && solut_lexems_it != solut_lexems.cend()
			 ; ++answr_lexems_it, ++solut_lexems_it)
		{
			if (answr_lexems_it->str != solut_lexems_it->str) {
				v.errors[line_num].insert(std::pair<int, std::string>(answr_lexems_it->pos, answr_lexems_it->str));
				v.state = MARK::ERROR;
				break;
			}
		}

		if (v.state != MARK::ERROR) {
			// not full answer
			if (solut_lexems_it != solut_lexems.cend()) {
				v.errors[line_num].insert(std::pair<int, std::string>(utf8_length(*answr_line_it), "..."));
				v.state = MARK::NOT_FULL_ANSWER;
			}

			// redundant answer
			if (answr_lexems_it != answr_lexems.cend()) {
				for (; answr_lexems_it != answr_lexems.cend(); ++answr_lexems_it) {
					v.errors[line_num].insert(std::pair<int, std::string>(answr_lexems_it->pos, answr_lexems_it->str));
				}
			}
		}
	}

	return v;
}

} // namespace analysis

