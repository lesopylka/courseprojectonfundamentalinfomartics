#ifndef MAP_H_MULTIPLE_REQUEST_H
#define MAP_H_MULTIPLE_REQUEST_H


#include "../extensions/serializable.h"
#include "memory_connection.h"


class MultipleRequest : public Connection
{
private:

	std::shared_ptr<Connection> connection;
	bool status = false; // false - 0 ok requests
	int waitResponseCount;

public:

	MultipleRequest(std::shared_ptr<Connection> connection, int waitResponseCount)
			: connection(std::move(connection)), waitResponseCount(waitResponseCount)
	{
		if (waitResponseCount < 1)
			throw std::runtime_error("Response count must be > 0");
	}

	// returns is the required number of responses received
	bool getResponse(bool status)
	{
		if (status)
			this->status = true;
		waitResponseCount--;
		if (waitResponseCount < 1)
			return true;
		return false;
	}

	std::shared_ptr<Connection> getConnection()
	{
		return connection;
	}

	bool getStatus() const
	{
		return status;
	}

	const char* receiveMessage() const override
	{
		return connection->receiveMessage();
	}

	void sendMessage(const Serializable& message) const override
	{
		return connection->sendMessage(message);
	}

	MultipleRequest(const MultipleRequest&) = delete;

	MultipleRequest& operator=(const MultipleRequest&) = delete;

	MultipleRequest(MultipleRequest&&) = delete;

	MultipleRequest& operator=(MultipleRequest&&) = delete;
};


#endif //MAP_H_MULTIPLE_REQUEST_H
