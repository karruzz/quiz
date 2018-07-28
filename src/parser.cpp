#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <numeric>
#include <functional>

#include <boost/filesystem/operations.hpp>

#include "parser.h"

#define USE_TOPICS "-t"
#define TRIM_TYPE(line) line.erase(0, 1);

namespace {

// question file structs
const char COMMENT   = '#';
const char TOPIC     = '%';
const char HASH      = '^';
const char TAG       = '^';  // deprecated
const char QUESTION  = '>';
const char ANSWER    = '<';
const char BLOCK     = '@'; // for repeated text blocks

// statistic file structs
const char STATISTIC_QUESTION = '^';
const char STATISTIC_VALUES   = '>';

enum parse_state {
	STATE_NONE,
	STATE_ANSWER_PREPARING,
	STATE_QUESTION_PREPARING,
	STATE_STATISTIC_PREPARING,
	STATE_HASH_PREPARING,
	STATE_BLOCK_PREPARING
};

enum line_type {
	LINE_ANSWER,
	LINE_COMMENT,
	LINE_EMPTY,
	LINE_NO_TYPE,  // no type at 0 character
	LINE_QUESTION,
	LINE_TAG,
	LINE_TOPIC,
	LINE_BLOCK
};

#define LINE(X) {X, LINE_##X}

std::map<char, line_type> line_types = { LINE(ANSWER), LINE(COMMENT), LINE(QUESTION), LINE(TOPIC), LINE(TAG), LINE(BLOCK) };

line_type get_line_type(const std::string &line)
{
	if (line.empty())
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
	bool was_attempt;

	Statistic(size_t question_hash, int total_errors, int last_errors, bool was_attempt)
		: question_hash(question_hash), total_errors(total_errors), last_errors(last_errors), was_attempt(was_attempt) {}

	Statistic(const Statistic& p) = default;
	Statistic& operator= (const Statistic& p) = default;

	bool operator< (const Statistic &p) const {
		return question_hash < p.question_hash;
	}
};


int is_space_not_tab(int c) {
	return static_cast<int>(std::isspace(c) && c != '\t');
}

inline void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(),
			std::not1(std::ptr_fun<int, int>(is_space_not_tab))));
}

inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(),
			std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

inline void trim_spaces(std::string &s) {
	ltrim(s);
	rtrim(s);
}

}

static
std::map<size_t, Statistic> load_statistic(const boost::filesystem::path &lrn_file_path)
{
	std::map<size_t, Statistic> result;

	boost::filesystem::path stat_file_path = "." + lrn_file_path.stem().string() + ".stat";
	stat_file_path = lrn_file_path.parent_path() / stat_file_path;

	if (!boost::filesystem::exists(stat_file_path) || !boost::filesystem::is_regular_file(stat_file_path))
		return result;

	std::ifstream stat_ifstream(stat_file_path.string());
	if (!stat_ifstream.is_open())
		return result;

	parse_state previous_state = STATE_NONE;
	std::string line, question_hash_line;
	while (std::getline(stat_ifstream, line)) {
		char line_type = line.at(0);
		TRIM_TYPE(line);
		trim_spaces(line);

		if (line_type == STATISTIC_QUESTION) {
			question_hash_line = line;
			previous_state = STATE_HASH_PREPARING;
		} else if (line_type == STATISTIC_VALUES) {
			assert(previous_state == STATE_HASH_PREPARING);

			int total_errors, last_errors;
			bool was_attempt;
			std::istringstream stat_ss(line);
			stat_ss >> total_errors >> last_errors >> was_attempt;

			size_t question_hash;
			std::istringstream tag_ss(question_hash_line);
			if (tag_ss >> question_hash) {
#ifdef DEBUG
				std::cout << "line: " << question_hash_line << "; hash: " << question_hash;
				std::cout << "; total: " << total_errors << "; last: " << last_errors;
				std::cout << "; was attempts: " << was_attempt << std::endl;
#endif
				result.insert(std::pair<size_t, Statistic>(
					question_hash, Statistic(question_hash, total_errors, last_errors, was_attempt)));
			}

			previous_state = STATE_STATISTIC_PREPARING;
		} else {
			throw std::runtime_error("invalid statistic line type");
		}
	}

	stat_ifstream.close();
	return result;
}

void Parser::save_statistic(const std::vector<Problem> &problems, const boost::filesystem::path &path)
{
	boost::filesystem::path stat_path = "." + path.stem().string() + ".stat";
	stat_path = path.parent_path() / stat_path;

	std::ofstream of(stat_path.string());
	if(!of.is_open()) {
		std::cerr << "Cannot create statisics file" << std::endl;
		return;
	}

	std::vector<Problem>::const_iterator it = problems.begin();
	for (; it != problems.end(); ++it) {
		of << HASH << " " << it->question_hash << std::endl;
		of << STATISTIC_VALUES << " " << it->total_errors << " ";
		of << (it->was_attempt_this_time ? it->errors : it->last_errors) << " ";
		of << it->was_attempt_any_time_before << std::endl;
	}

	of.flush();
	of.close();
}

std::vector<Problem> Parser::load(
		const boost::filesystem::path &lrn_file_path,
		std::map<std::string, std::vector<std::string> > params)
{
	std::vector<Problem> problems;
	std::map<std::string, std::list<std::string>> repeat_blocks;

	if (!boost::filesystem::exists(lrn_file_path) || !boost::filesystem::is_regular_file(lrn_file_path))
		return problems;

	std::ifstream lrn_file_ifstream(lrn_file_path.string());
	if (!lrn_file_ifstream.is_open())
		return problems;

	bool use_topics = params.find(USE_TOPICS) != params.end();
	bool desired_topic = false;
	parse_state prev_state = STATE_NONE, state = STATE_NONE;
	std::string line, block_name;
	std::list<std::string> quest, answ, block;
	int line_num = -1;
	while (std::getline(lrn_file_ifstream, line)) {
		line_num++;
		line_type t = get_line_type(line);

		if (t == LINE_TOPIC) {
			if (!use_topics)
				continue;

			std::string topic = line.substr(1);
			trim_spaces(topic);
			std::vector<std::string> &desired_topics = params[USE_TOPICS];
			desired_topic =
				std::find(desired_topics.begin(), desired_topics.end(), topic) != desired_topics.end();
			continue;
		}

		if (use_topics && !desired_topic)
			continue;

		switch (t) {
			case (LINE_EMPTY) :
			case (LINE_COMMENT) :
			case (LINE_TAG):
				continue;
			case (LINE_NO_TYPE):
				break;
			case (LINE_ANSWER) :
				state = STATE_ANSWER_PREPARING;
				TRIM_TYPE(line);
				break;
			case (LINE_QUESTION) :
				state = STATE_QUESTION_PREPARING;
				TRIM_TYPE(line);
				break;
			case (LINE_BLOCK) :
				state = STATE_BLOCK_PREPARING;
				TRIM_TYPE(line);
				break;
			default:
				break;
		}

		if (state != prev_state) {
			if (prev_state == STATE_ANSWER_PREPARING) {
				if (quest.size() > 0 && answ.size() > 0) {
					problems.push_back(Problem(quest, answ, 1));

					quest.clear();
					answ.clear();
				}
			} else if (prev_state == STATE_BLOCK_PREPARING) {
				repeat_blocks.insert(std::pair<std::string, std::list<std::string>>(block_name, block));
				block_name.clear();
				block.clear();
			}

			prev_state = state;
		}

		trim_spaces(line);

		if (state == STATE_ANSWER_PREPARING) {
			answ.push_back(line);
		} else if (state == STATE_QUESTION_PREPARING) {
			size_t block_start = line.find("{{");
			size_t block_end = line.find("}}");

			if (block_start != std::string::npos && block_end != std::string::npos) {
				std::string block_name = line.substr(block_start + 2, block_end - (block_start + 2));
				auto it = repeat_blocks.find(block_name);
				if (it != repeat_blocks.end())
					quest.insert(quest.end(), it->second.begin(), it->second.end());
				else
					quest.push_back(line);
			} else
				quest.push_back(line);
		} else if (state == STATE_HASH_PREPARING) {
		} else if (state == STATE_QUESTION_PREPARING) {
			quest.push_back(line);
		} else if (state == STATE_BLOCK_PREPARING) {
			if (block_name.empty())
				block_name = line;
			else
				block.push_back(line);
		}
	}

	if (quest.size() > 0 && answ.size() > 0) {
		problems.push_back(Problem(quest, answ, 1));
	}

	lrn_file_ifstream.close();

	for (auto it = problems.begin(); it != problems.end(); ++it) {
		std::string s = std::accumulate(it->question.begin(), it->question.end(), std::string(""));
		it->question_hash = std::hash<std::string>()(s);
	}

	std::map<size_t, Statistic> stat = load_statistic(lrn_file_path);

	for (auto it = problems.begin(); it != problems.end(); ++it) {
		std::map<size_t, Statistic>::iterator mit = stat.find(it->question_hash);
		if (mit != stat.end()) {
			it->total_errors = mit->second.total_errors;
			it->last_errors = mit->second.last_errors;
			it->was_attempt_any_time_before = mit->second.was_attempt;
		}
	}

	return problems;
}

// returns line numbers and position on line with error
std::map<int, int> Parser::check_answer(
		const std::list<std::string> &user_answer,
		const std::list<std::string> &right_answer)
{
	std::map<int, int> errors;
	std::list<std::string>::const_iterator user_it = user_answer.cbegin();
	std::list<std::string>::const_iterator right_it = right_answer.cbegin();

	int line_num = 0;
	for ( ; user_it != user_answer.cend() && right_it != right_answer.cend(); ++user_it, ++right_it, ++line_num) {
		for (size_t i = 0; i < right_it->size(); i++) {
			if (i >= user_it->size() || right_it->at(i) != user_it->at(i)) {
				errors[line_num] = i;
				break;
			}
		}
	}

	return errors;
}
