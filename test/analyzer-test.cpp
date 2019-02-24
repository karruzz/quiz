/*
 * analyzer-test.cpp
 *
 *  Created on: Jan 01, 2019
 *  Copyright © 2018-2081 Ilja Karasev <ilja.karasev@gmail.com>.
 *  All rights reserved.
 *     License: GNU GPL 3
 */

#include <iostream>
#include "gtest/gtest.h"

#include "analyzer.h"
#include "problem.h"
#include "utils.h"

namespace {

namespace an = analysis;

TEST (AnalyzeTest, Spelling)
{
	Problem p;
	p.question = { "Hello world" };
	p.solution = { "Привет мир" };
	std::list<std::string> answer = { "превет" };

	an::Analyzer a;
	an::Verification v = a.check(p, answer, an::Analyzer::OPTIONS::CASE_UNSENSITIVE);
	ASSERT_EQ ((size_t)1, v.errors.size());

	std::list<an::Error> le = v.errors.at(0);
	std::vector<an::Error> e(std::begin(le), std::end(le));
	ASSERT_EQ ((size_t)3, e.size());

	EXPECT_EQ (an::MARK::ERROR | an::MARK::NOT_FULL_ANSWER, v.state);

	EXPECT_EQ (an::Error::WHAT::ERROR_LEXEM, e.at(0).what);
	EXPECT_EQ ((size_t)0, e.at(0).pos);
	EXPECT_EQ ("превет", to_utf8(e.at(0).str));

	EXPECT_EQ (an::Error::WHAT::ERROR_SYMBOL, e.at(1).what);
	EXPECT_EQ ((size_t)2, e.at(1).pos);
	EXPECT_EQ ("е", to_utf8(e.at(1).str));

	EXPECT_EQ (an::Error::WHAT::MISSED, e.at(2).what);
	EXPECT_EQ ((size_t)6, e.at(2).pos);
	EXPECT_EQ ("...", to_utf8(e.at(2).str));
}

TEST (AnalyzeTest, Punctuation)
{
	Problem p;
	p.question = { "Hello, world!" };
	p.solution = { "Привет, мир!" };
	std::list<std::string> answer = { "Привет мир" };

	an::Analyzer a;
	an::Verification v = a.check(p, answer, an::Analyzer::OPTIONS::PUNCT_UNSENSITIVE);
	EXPECT_EQ ((size_t)0, v.errors.size());
}

}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
