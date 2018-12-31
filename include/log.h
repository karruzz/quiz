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
			std::cout << stream.str();
	}

	std::stringstream stream;
	int uncaught;
};

class LogInfo : public Message
{
public:
	LogInfo() : Message("[INFO] ") {}
};

class LogWarning : public Message
{
public:
	LogWarning() : Message("[WARN] ") {}
};

class Error : public Message
{
public:
	Error() : Message("[ERROR] ") {}
};

template <typename T>
Message& operator<<(Message& record, T&& t) {
	record.stream << record.prefix << std::forward<T>(t);
	return record;
}

template <typename T>
Message& operator<<(Message&& record, T&& t) {
	return record << std::forward<T>(t);
}

} // namespace log

#endif // LOG_H
