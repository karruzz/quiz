/*
 * log.h
 *
 *  Created on: Dec 31, 2018
 *  Copyright © 2018-2081 Ilja Karasev <ilja.karasev@gmail.com>.
 *  All rights reserved.
 *     License: GNU GPL 3
 */

#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <sstream>
#include <exception>
#include <stdexcept>

namespace logging {

extern std::string endl;

class Message
{
public:
	const std::string prefix;

	Message() {
		uncaught = std::uncaught_exception();
	}

	Message(const std::string& prefix_) : prefix(prefix_) {
		uncaught = std::uncaught_exception();
	}

	~Message() {
		if (uncaught >= std::uncaught_exception())
			std::cout << prefix << stream.str();
	}

	std::stringstream stream;
	int uncaught;
};


class Info : public Message
{
public:
	Info() : Message("[INFO] ") {}
};


class Warning : public Message
{
public:
	Warning() : Message("[WARN] ") {}
};


class Error : public Message
{
public:
	Error() : Message("[ERROR] ") {}
};


template <typename T>
Message& operator<<(Message& record, T&& t) {
	record.stream << std::forward<T>(t);
	return record;
}


template <typename T>
Message& operator<<(Message&& record, T&& t) {
	return record << std::forward<T>(t);
}

} // namespace log

#endif // LOG_H
