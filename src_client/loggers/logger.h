#ifndef SANDBOX_CPP_LOGGER_H
#define SANDBOX_CPP_LOGGER_H

#include <iostream>

class logger
{

public:

	enum class severity
	{
		trace,
		debug,
		information,
		warning,
		error,
		critical
	};

public:

	virtual logger const* log(const std::string&, severity) const = 0;

	virtual ~logger();

};

#endif //SANDBOX_CPP_LOGGER_H
