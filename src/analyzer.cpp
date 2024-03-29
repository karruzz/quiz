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
#include <options.h>
#include <utils.h>

namespace analysis {

#define TAB_WIDTH 4

static
Token::WHAT what_token(char16_t c)
{
	static const std::set<char16_t> space_tokens = { ' ', '\t' };
	static const std::set<char16_t> punctuate_tokens
		= { ',', '.', '?', '!', '-', ':', ';', '_', '(', ')',
			'[', ']', '<', '>', '{', '}', '+', '-', '=', '*', '/' };
	static const std::set<char16_t> delimeter_tokens
		= { ',', '.', '?', '!', ':', ';' };

	if (space_tokens.find(c) != space_tokens.end())
		return Token::SPACE;

	if (punctuate_tokens.find(c) != punctuate_tokens.end()) {
		int result = Token::PUNCT;
		if (delimeter_tokens.find(c) != delimeter_tokens.end())
			result |= Token::DELIM;
		return (Token::WHAT)result;
	}

	return Token::WORD;
}

std::list<Token> Analyzer::split_to_tokens(const std::string& s)
{
	std::list<Token> tokens;
	if (s.empty()) return tokens;

	std::u16string token_str;
	size_t position = 0;

	std::u16string line = utils::to_utf16(s);
	std::u16string::iterator c = line.begin(), next = std::next(line.begin());
	for (; c != line.end(); ++c, ++next) {
		Token::WHAT token_type = what_token(*c);
		if (token_type == Token::WORD)
			token_str.push_back(*c);
		else
			token_str = *c;

		Token::WHAT token_type_next = next != line.end() ? what_token(*next) : Token::UNDEF;
		if (token_type != token_type_next) {
			tokens.push_back({ token_type, token_str, position});
			position += token_str.length();
			token_str.clear();
		}
	}

	return tokens;
}

static
Verification total_recall_check(Verification& v, bool case_unsensitive)
{
	std::set<std::string> recall_answ_set;
	std::set<std::string> recall_solut_set;

	auto remove_space = [](const Token& l){ return l.what == Token::SPACE; };
	auto downcase = [](Token& l) { utils::to_lower(l.str); return l; };
	auto split_to_phrases = [] (std::set<std::string>& set, const std::list<Token>& tokens) {
		std::string phrase;
		std::list<Token>::const_iterator it = tokens.begin(), next = std::next(tokens.begin());
		for (; it != tokens.end(); ++it, ++next) {
			if (it->what & Token::WHAT::DELIM)
				continue;

			phrase.append(utils::to_utf8(it->str));
			if (next == tokens.end() || (next->what & Token::WHAT::DELIM)) {
				utils::trim_spaces(phrase);
				set.insert(phrase);
				phrase.clear();
			}
		}
	};


	auto analyze_lines = [case_unsensitive, remove_space, downcase, split_to_phrases]
			(std::set<std::string>& set, const std::list<std::string> &lines)
	{
		for (auto line = lines.cbegin(); line != lines.cend(); ++line) {
			std::list<Token> tokens = Analyzer::split_to_tokens(*line);

			if (case_unsensitive)
				std::transform(tokens.begin(), tokens.end(), tokens.begin(), downcase);

			split_to_phrases(set, tokens);
		}
	};

	analyze_lines(recall_answ_set, v.answer);
	analyze_lines(recall_solut_set, v.solution);

	std::vector<std::string> wrong, missed;
	std::set_difference(
		recall_answ_set.cbegin(), recall_answ_set.cend(),
		recall_solut_set.cbegin(), recall_solut_set.cend(),
		std::inserter(wrong, wrong.begin()));

	std::set_difference(
		recall_solut_set.cbegin(), recall_solut_set.cend(),
		recall_answ_set.cbegin(), recall_answ_set.cend(),
		std::inserter(missed, missed.begin()));

	std::list<std::string> solution;
	if (!wrong.empty()) {
		solution.push_back("Wrong:");
		solution.push_back("");
		for (const std::string &w: wrong) {
			if (solution.back().size() > 80) solution.push_back("");
			solution.back().append(w + ", ");
		}
		solution.push_back("");
	}

	if (!missed.empty()) {
		solution.push_back("Missed:");
		solution.push_back("");
		for (const std::string &m: missed) {
			if (solution.back().size() > 80) solution.push_back("");
			solution.back().append(m + ", ");
		}
	}

	if (!solution.empty()) {
		v.state |= MARK::ERROR;
		v.solution =  solution;
	}

	return v;
}

Verification Analyzer::check(
	const Problem& problem, const std::list<std::string>& answer, const Options& options)
{
	Verification v(problem, answer);
	v.answer.remove("");

	std::for_each(v.answer.begin(), v.answer.end(), utils::trim_spaces);
	std::for_each(v.answer.begin(), v.answer.end(), utils::remove_duplicate_spaces);

	if (options.get(Options::ANALYSIS_TOTAL_RECALL))
		return total_recall_check(v, options.get(Options::ANALYSIS_CASE_UNSENSITIVE));

	int line_num = 0;
	auto answr_line = v.answer.cbegin();
	auto solut_line = v.solution.cbegin();
	for (; answr_line != v.answer.cend() && solut_line != v.solution.cend()
		 ; ++answr_line, ++solut_line, ++line_num)
	{
		auto answr_tokens = split_to_tokens(*answr_line);
		auto solut_tokens = split_to_tokens(*solut_line);

		auto remove_space = [](const Token& l){ return l.what == Token::SPACE; };
		answr_tokens.remove_if(remove_space);
		solut_tokens.remove_if(remove_space);

		if (options.get(Options::ANALYSIS_CASE_UNSENSITIVE)) {
			auto downcase = [](Token& l) { utils::to_lower(l.str); return l; };
			std::transform(answr_tokens.begin(), answr_tokens.end(), answr_tokens.begin(), downcase);
			std::transform(solut_tokens.begin(), solut_tokens.end(), solut_tokens.begin(), downcase);
		}

		if (options.get(Options::ANALYSIS_PUNTCTUATION_UNSENSITIVE)) {
			auto remove_punctuation = [](const Token& l){ return l.what & Token::PUNCT; };
			answr_tokens.remove_if(remove_punctuation);
			solut_tokens.remove_if(remove_punctuation);
		}

		auto answr_token = answr_tokens.cbegin();
		auto solut_token = solut_tokens.cbegin();
		for (; answr_token != answr_tokens.cend() && solut_token != solut_tokens.cend()
			 ; ++answr_token, ++solut_token)
		{
			std::u16string answr = answr_token->str;
			std::u16string solut = solut_token->str;
			size_t pos = answr_token->pos;

			std::u16string answr_next;
			auto answr_it_next = std::next(answr_token);
			if (answr_it_next != answr_tokens.cend())
				answr_next = answr_it_next->str;

			std::u16string solut_next;
			auto solut_it_next = std::next(solut_token);
			if (solut_it_next != solut_tokens.cend())
				solut_next = solut_it_next->str;

			if (answr != solut) {
				if (pos != 0 && answr == solut_next) { // token missed in answer
					++solut_token;
					v.errors[line_num].push_back({Error::MISSED, utils::to_utf16(" "), pos - 1});
				} else if (answr_next == solut) { // redundant token in answer
					++answr_token;
					v.errors[line_num].push_back({Error::REDUNDANT, answr, pos});
				} else { // error token
					v.errors[line_num].push_back({Error::ERROR_TOKEN, answr, pos});

					for (size_t i = 0; i < answr.size() && i < solut.size(); ++i)
						if (answr.at(i) != solut.at(i)) {
							v.errors[line_num].push_back(
								{Error::ERROR_SYMBOL, std::u16string(1, answr.at(i)), pos + i});
						}

					// different length
					if (solut.size() < answr.size())
						v.errors[line_num].push_back(
							{Error::ERROR_SYMBOL, answr.substr(solut.size()), pos + solut.size()});
				}
				v.state |= MARK::ERROR;
			}
		}

		// not full answer
		if (solut_token != solut_tokens.cend()) {
			v.errors[line_num].push_back(
				{Error::MISSED, utils::to_utf16("..."), utils::to_utf16(*answr_line).size()});
			v.state |= MARK::NOT_FULL_ANSWER;
		}

		// redundant answer
		if (answr_token != answr_tokens.cend()) {
			for (; answr_token != answr_tokens.cend(); ++answr_token)
				v.errors[line_num].push_back(
					{Error::REDUNDANT, answr_token->str, answr_token->pos});
			v.state |= MARK::REDUNDANT_ANSWER;
		}
	}

	if(options.get(Options::ANALYSIS_TOTAL_RECALL) && v.solution.size() != v.answer.size())
		v.state |= MARK::INVALID_LINES_NUMBER;

	return v;
}

} // namespace analysis

