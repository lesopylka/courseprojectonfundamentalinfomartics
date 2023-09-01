#ifndef PROGC_SERVER_LOGGER_H
#define PROGC_SERVER_LOGGER_H


#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <thread>
#include <queue>
#include "../../connection/connection.h"
#include "../../connection/memory_connection.h"
#include "../../data_types/shared_object.h"
#include "../../processors/processor.h"
#include "../logger.h"


using namespace boost::interprocess;


class ServerLogger : public Processor
{
private:

	const int serverStatusCode;
	const Connection* connection;
	named_mutex* mutex;
	std::queue<std::string> toProcess;

public:

	ServerLogger(const int serverStatusCode, const std::string& memNameForLog,
			const std::string& mutexNameForLog) : serverStatusCode(serverStatusCode)
	{
		connection = new MemoryConnection(false, memNameForLog);
		mutex = new named_mutex(open_only, mutexNameForLog.c_str());
	}

	~ServerLogger() override
	{
		delete connection;
		delete mutex;
	}

	void log(const std::string& string, logger::severity severity)
	{
		std::stringstream ss;
		ss << std::string(reinterpret_cast<const char* const>(&severity), sizeof(severity));
		size_t tmp = string.length();
		ss << std::string(reinterpret_cast<char*>(&tmp), sizeof(tmp)) << string;
		toProcess.emplace(ss.str());
	}

	void logSync(const std::string& string, logger::severity severity)
	{
		std::stringstream ss;
		ss << std::string(reinterpret_cast<const char* const>(&severity), sizeof(severity));
		size_t tmp = string.length();
		ss << std::string(reinterpret_cast<char*>(&tmp), sizeof(tmp)) << string;
		scoped_lock<named_mutex> lock(*mutex);
		while (SharedObject::getStatusCode(connection->receiveMessage()) != serverStatusCode)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		connection->sendMessage(SharedObject(serverStatusCode + 1,
				SharedObject::RequestResponseCode::LOG, ss.str()));
	}

	void process() override
	{
		if (toProcess.empty())
			return;
		if (!mutex->try_lock())
			return;
		if (SharedObject::getStatusCode(connection->receiveMessage()) == serverStatusCode)
		{
			connection->sendMessage(SharedObject(serverStatusCode + 1,
					SharedObject::RequestResponseCode::LOG, toProcess.front()));
			toProcess.pop();
		}
		mutex->unlock();
	}
};


#endif //PROGC_SERVER_LOGGER_H
