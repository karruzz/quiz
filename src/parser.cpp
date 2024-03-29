/*
 * parser.cpp
 *
 *  Created on: May 19, 2018
 *  Copyright © 2018-2081 Ilja Karasev <ilja.karasev@gmail.com>.
 *  All rights reserved.
 *     License: GNU GPL 3
 */

#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <set>
#include <numeric>
#include <functional>
#include <filesystem>
#include <cassert>

#include <parser.h>
#include <utils.h>
#include <log.h>

namespace {

namespace fs = std::filesystem;

// question file structs
const char COMMENT   = '#';
const char TOPIC     = '%';
const char HASH      = '^';
const char TAG       = '^';  // deprecated
const char QUESTION  = '>';
const char SOLUTION  = '<';
const char BLOCK     = '@'; // for repeated text blocks
const char LANGUAGE  = '&';

// statistic file structs
const char STATISTIC   = '>';

enum parse_state {
	STATE_NONE,
	STATE_SOLUTION_PREPARING,
	STATE_QUESTION_PREPARING,
	STATE_STATISTIC_PREPARING,
	STATE_HASH_PREPARING,
	STATE_BLOCK_PREPARING
};

enum line_type {
	LINE_SOLUTION,
	LINE_COMMENT,
	LINE_EMPTY,
	LINE_NO_TYPE,  // no type at 0 character
	LINE_QUESTION,
	LINE_TAG, // deprecated
	LINE_TOPIC,
	LINE_BLOCK,
	LINE_LANGUAGE,
};

#define LINE(X) {X, LINE_##X}

std::map<char, line_type> line_types = { LINE(SOLUTION), LINE(COMMENT), LINE(QUESTION), LINE(TOPIC), LINE(TAG), LINE(BLOCK), LINE(LANGUAGE) };

line_type get_line_type(const std::string& line)
{
	if (line.empty() || line == "\r")
		return LINE_EMPTY;

	std::map<char, line_type>::iterator it = line_types.find(line.at(0));
	if (it != line_types.end())
		return it->second;

	return LINE_NO_TYPE;
}

struct Statistic {
	size_t question_hash;
	int total_errors;
	int last_errors;

	Statistic(size_t question_hash, int total_errors, int last_errors)
		: question_hash(question_hash), total_errors(total_errors), last_errors(last_errors) {}

	Statistic(const Statistic& p) = default;
	Statistic& operator= (const Statistic& p) = default;

	bool operator< (const Statistic &p) const {
		return question_hash < p.question_hash;
	}
};

void trim_type(std::string& s)
{
	s.erase(0, 1);
}

std::map<size_t, Statistic> load_statistic(const fs::path& quiz_path)
{
	std::map<size_t, Statistic> result;

	fs::path stat_path = "." + quiz_path.stem().string() + ".stat";
	stat_path = quiz_path.parent_path() / stat_path;

	if (!fs::exists(stat_path) || !fs::is_regular_file(stat_path))
		return result;

	std::ifstream stat_ifstream(stat_path.string());
	if (!stat_ifstream.is_open())
		return result;

	parse_state previous_state = STATE_NONE;
	std::string line, question_hash_line;
	while (std::getline(stat_ifstream, line)) {
		if (line.empty())
			continue;

		char line_type = line.at(0);
		trim_type(line);
		utils::trim_spaces(line);

		if (line_type == HASH) {
			question_hash_line = line;
			previous_state = STATE_HASH_PREPARING;
		} else if (line_type == STATISTIC) {
			assert(previous_state == STATE_HASH_PREPARING);

			int total_errors, last_errors;
			std::istringstream stat_ss(line);
			stat_ss >> total_errors >> last_errors;

			size_t question_hash;
			std::istringstream tag_ss(question_hash_line);
			if (tag_ss >> question_hash) {
#ifdef DEBUG
				logging::Message() << "line: " << question_hash_line << "; hash: " << question_hash;
				logging::Message() << "; total: " << total_errors << "; last: " << last_errors << logging::endl;
#endif
				result.insert(std::pair<size_t, Statistic>(
					question_hash, Statistic(question_hash, total_errors, last_errors)));
			}

			previous_state = STATE_STATISTIC_PREPARING;
		} else if (line_type == COMMENT) {
			continue;
		} else {
			throw std::runtime_error("invalid statistic line type");
		}
	}

	stat_ifstream.close();
	return result;
}

} // namespace

void Parser::save_statistic(const std::vector<std::shared_ptr<Problem>>& problems, const fs::path& path)
{
	fs::path stat_path = "." + path.stem().string() + ".stat";
	stat_path = path.parent_path() / stat_path;

	std::ofstream of(stat_path.string());
	if(!of.is_open()) {
		logging::Error() << "Cannot create statisics file" << logging::endl;
		return;
	}

	of << COMMENT <<  " > 2 0: total_errors - 2, last_errors - 0" << std::endl;
	of << std::endl;
	for (const auto p: problems) {
		of << COMMENT << " " << p->question().front() << std::endl;
		of << HASH << " " << p->question_hash << std::endl;
		of << STATISTIC << " " << p->total_errors << " ";
		of << (p->was_attempt ? p->errors : p->last_errors) << std::endl;
		of << std::endl;
	}

	of.flush();
	of.close();
}

std::vector<std::shared_ptr<Problem>> Parser::load(const Options& options)
{
	std::vector<std::shared_ptr<Problem>> problems;
	std::map<std::string, std::list<std::string>> repeat_blocks;

	if (!fs::exists(options.filename()) || !fs::is_regular_file(options.filename()))
		return problems;

	std::ifstream quiz_ifstream(options.filename());
	if (!quiz_ifstream.is_open())
		return problems;

	bool needed_topic = false;
	std::set<std::string> topics;
	if (options.get(Options::USE_TOPICS)) {
		auto from_params = options.args(Options::USE_TOPICS);
		std::copy(from_params.begin(), from_params.end(), std::inserter(topics, topics.begin()));
	}

	parse_state prev_state = STATE_NONE, state = STATE_NONE;
	std::string line;
	std::list<std::string> quest, solut, block;
	int question_number = -1, questions_loaded = 0;

	std::set<std::string> topics_in_quiz;
	utils::Language question_language = utils::Language::UNKNOWN;
	utils::Language solution_language = utils::Language::UNKNOWN;
	while (std::getline(quiz_ifstream, line)) {
		prev_state = state;
		line_type t = get_line_type(line);

		if (t == LINE_EMPTY || t == LINE_COMMENT || t == LINE_TAG)
			continue;

		// prepare line
		if (t != LINE_NO_TYPE) trim_type(line);
		utils::trim_spaces(line);
		utils::remove_duplicate_spaces(line);

		if (t == LINE_TOPIC) {
			topics_in_quiz.insert(line);
			needed_topic = options.get(Options::USE_TOPICS) && topics.find(line) != topics.end();
			continue;
		} else if (t == LINE_LANGUAGE) {
			auto detectLanguage = [](const std::string& s) {
				if (s == "ru")
					return utils::Language::RU;
				if (s == "en")
					return utils::Language::EN;
				if (s == "nl")
					return utils::Language::NL;

				return utils::Language::UNKNOWN;
			};

			auto langs = utils::split(line, " ");
			question_language = detectLanguage(langs[0]);
			solution_language = detectLanguage(langs[1]);
			continue;
		} else if (t == LINE_SOLUTION) {
			state = STATE_SOLUTION_PREPARING;
		} else if (t == LINE_QUESTION) {
			state = STATE_QUESTION_PREPARING;
		} else if (t == LINE_BLOCK) {
			state = STATE_BLOCK_PREPARING;
		}

		bool state_changed = state != prev_state;

		// flush block
		if (state_changed && prev_state == STATE_BLOCK_PREPARING) {
			std::string block_name = block.front();
			block.pop_front();
			repeat_blocks.insert(std::pair<std::string, std::list<std::string>>(block_name, block));
			block.clear();
		}

		if (state == STATE_BLOCK_PREPARING)
			block.push_back(line);

		if (options.get(Options::USE_TOPICS) && !needed_topic)
			continue;

		// flush problem
		if (state_changed && prev_state == STATE_SOLUTION_PREPARING) {
			if (quest.size() > 0 && solut.size() > 0) {
				++question_number;
				problems.push_back(std::make_shared<Problem>(quest, solut, question_language, solution_language));
				questions_loaded++;

				quest.clear();
				solut.clear();
			}
		}

		if (state == STATE_SOLUTION_PREPARING) {
			solut.push_back(line);
		} else if (state == STATE_QUESTION_PREPARING) {
			size_t block_start = line.find("{{");
			size_t block_end = line.find("}}");

			if (block_start != std::string::npos && block_end != std::string::npos) {
				std::string block_name = line.substr(block_start + 2, block_end - (block_start + 2));
				auto repeat_block = repeat_blocks.at(block_name);
				quest.insert(quest.end(), repeat_block.begin(), repeat_block.end());
			} else
				quest.push_back(line);
		}
	}

	if (quest.size() > 0 && solut.size() > 0) {
		problems.push_back(std::make_shared<Problem>(quest, solut, question_language, solution_language));
	}

	quiz_ifstream.close();

	for (std::shared_ptr<Problem> p: problems) {
		std::string s = std::accumulate(p->question().begin(), p->question().end(), std::string(""));
		p->question_hash = std::hash<std::string>()(s);
	}

	std::map<size_t, Statistic> stat = load_statistic(options.filename());

	for (std::shared_ptr<Problem> p: problems) {
		std::map<size_t, Statistic>::iterator mit = stat.find(p->question_hash);
		if (mit != stat.end()) {
			p->total_errors = mit->second.total_errors;
			p->last_errors = mit->second.last_errors;
		}
	}

	if (options.get(Options::USE_TOPICS))
		std::for_each(topics.cbegin(), topics.cend(), [&topics_in_quiz] (const std::string& s) {
			bool any_lost_topics = false;
			if (topics_in_quiz.find(s) == topics_in_quiz.end()) {
				logging::Error() << "Topic <" + s + "> not found in quiz" << logging::endl;
				any_lost_topics = true;
			}
			if (any_lost_topics)
				throw std::runtime_error("not all topics were found in quiz, check command line arguments");
		}
	);

	return problems;
}

