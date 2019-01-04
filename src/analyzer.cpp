/*
 * analyzer.cpp
 *
 *  Created on: Dec 08, 2018
 *  Copyright © 2018-2081 Ilja Karasev <ilja.karasev@gmail.com>.
 *  All rights reserved.
 *     License: GNU GPL 3
 */

#include <algorithm>
#include <set>
#include <vector>

#include <analyzer.h>
#include <utils.h>

namespace analysis {

namespace {

struct Lexem
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
};


std::list<Lexem> split_to_lexemes(const std::string& s)
{
	static const std::set<char16_t> space_lexems = { ' ', '\t' };
	static const std::set<char16_t> punctuate_lexems
		= { ',', '.', '?', '!', '-', ':', ';', '_', '(', ')',
			'[', ']', '<', '>', '{', '}', '+', '-', '=', '*', '/' };

	std::list<Lexem> lexemes;
	std::u16string lexem;
	size_t position = 0;
	Lexem::WHAT prev_lexem_type = Lexem::UNDEF, lexem_type = Lexem::UNDEF;

	auto try_to_flush_lexem = [&]() {
		if (prev_lexem_type != Lexem::UNDEF && prev_lexem_type != lexem_type) {
			lexemes.push_back({ prev_lexem_type, lexem, position});
			position += lexem.length();
			lexem.clear();
		}
	};

	for (const char16_t& c: to_utf16(s)) {
		if (space_lexems.find(c) != space_lexems.end()) {
			lexem_type = Lexem::SPACE;
			try_to_flush_lexem();
			lexem = c;
		} else if (punctuate_lexems.find(c) != punctuate_lexems.end()) {
			lexem_type = Lexem::PUNCT;
			try_to_flush_lexem();
			lexem = c;
		} else {
			lexem_type = Lexem::WORD;
			try_to_flush_lexem();
			lexem.push_back(c);
		}

		prev_lexem_type = lexem_type;
	}

	// final lexem
	if (!lexem.empty())
		lexemes.push_back({ lexem_type, lexem, position });

	return lexemes;
}

} // namespace

Verification Analyzer::check(
	const Problem& problem, const std::list<std::string>& answer, int flags)
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
	auto answr_line = v.answer.cbegin();
	auto solut_line = v.solution.cbegin();
	for (; answr_line != v.answer.cend() && solut_line != v.solution.cend()
		 ; ++answr_line, ++solut_line, ++line_num)
	{
		auto answr_lexems = split_to_lexemes(*answr_line);
		auto solut_lexems = split_to_lexemes(*solut_line);

		answr_lexems.remove_if([](const Lexem& l){ return l.what == Lexem::SPACE; });
		solut_lexems.remove_if([](const Lexem& l){ return l.what == Lexem::SPACE; });

		if (flags & OPTIONS::PUNCT_UNSENSITIVE) {
			answr_lexems.remove_if([](const Lexem& l){ return l.what == Lexem::PUNCT; });
			solut_lexems.remove_if([](const Lexem& l){ return l.what == Lexem::PUNCT; });
		}

		auto answr_lexem = answr_lexems.cbegin();
		auto solut_lexem = solut_lexems.cbegin();
		for (; answr_lexem != answr_lexems.cend() && solut_lexem != solut_lexems.cend()
			 ; ++answr_lexem, ++solut_lexem)
		{
			std::u16string answr = answr_lexem->str;
			std::u16string solut = solut_lexem->str;
			std::u16string str = answr_lexem->str;
			size_t pos = answr_lexem->pos;

			if (flags & OPTIONS::CASE_UNSENSITIVE) {
				to_lower(answr);
				to_lower(solut);
			}

			if (answr != solut) {
				v.errors[line_num].push_back({Error::ERROR_LEXEM, str, pos});

				for (size_t i = 0; i < answr.size() && i < solut.size(); ++i)
					if (answr.at(i) != solut.at(i)) {
						v.errors[line_num].push_back(
							{Error::ERROR_SYMBOL, std::u16string(1, str.at(i)), pos + i});
					}

				// different length
				if (solut.size() < answr.size())
					v.errors[line_num].push_back(
						{Error::ERROR_SYMBOL, str.substr(solut.size()), pos + solut.size()});

				v.state |= MARK::ERROR;
			}
		}

		// not full answer
		if (solut_lexem != solut_lexems.cend()) {
			v.errors[line_num].push_back(
				{Error::ERROR_SYMBOL, to_utf16("..."), to_utf16(*answr_line).size()});
			v.state |= MARK::NOT_FULL_ANSWER;
		}

		// redundant answer
		if (answr_lexem != answr_lexems.cend()) {
			for (; answr_lexem != answr_lexems.cend(); ++answr_lexem)
				v.errors[line_num].push_back(
					{Error::ERROR_SYMBOL, answr_lexem->str, answr_lexem->pos});
			v.state |= MARK::REDUNDANT_ANSWER;
		}
	}

	return v;
}

} // namespace analysis

