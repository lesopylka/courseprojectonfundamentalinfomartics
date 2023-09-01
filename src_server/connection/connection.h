#ifndef PROGC_SRC_CONNECTION_CONNECTION_H
#define PROGC_SRC_CONNECTION_CONNECTION_H


#include <string>
#include <optional>
#include <boost/interprocess/mapped_region.hpp>
#include "../extensions/serializable.h"


class Connection
{
protected:

	std::string connectionName = "undefined";

public:

	const std::string& getName()
	{
		return connectionName;
	}

	virtual const char* receiveMessage() const = 0;

	virtual void sendMessage(const Serializable&) const = 0;

	virtual ~Connection() = default;
};


#endif //PROGC_SRC_CONNECTION_CONNECTION_H
