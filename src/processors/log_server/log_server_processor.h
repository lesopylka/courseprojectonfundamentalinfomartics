#ifndef PROGC_LOG_SERVER_PROCESSOR_H
#define PROGC_LOG_SERVER_PROCESSOR_H


#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <string>
#include "../../connection/connection.h"
#include "../../connection/memory_connection.h"
#include "../../data_types/shared_object.h"
#include "../../loggers/logger.h"
#include "../processor.h"


using namespace boost::interprocess;


class LogServerProcessor : public Processor
{
private:

	const int thisStatusCode;
	const Connection* connection;
	const named_mutex* mutex;
	const logger* logger;

public:

	LogServerProcessor(const int statusCode, const std::string& memNameForLog,
			const std::string& mutexNameForLog, const class logger* logger)
			: thisStatusCode(statusCode), logger(logger)
	{
		try
		{ named_mutex::remove(mutexNameForLog.c_str()); }
		catch (...)
		{}
		connection = new MemoryConnection(true, memNameForLog);
		mutex = new named_mutex(create_only, mutexNameForLog.c_str());
	}

	~LogServerProcessor() override
	{
		delete connection;
		delete mutex;
	}

	void process() override
	{
		if (SharedObject::getStatusCode(connection->receiveMessage()) == thisStatusCode)
			return;
		auto shared = SharedObject::deserialize(connection->receiveMessage());
		auto dataOpt = shared.getData();
		if (shared.getRequestResponseCode() != SharedObject::RequestResponseCode::LOG || !dataOpt)
		{
			connection->sendMessage(SharedObject(thisStatusCode,
					SharedObject::RequestResponseCode::ERROR, SharedObject::NULL_DATA));
			return;
		}
		const char* ptr = dataOpt.value().c_str();

		logger::severity severity = *reinterpret_cast<const logger::severity*>(ptr);
		ptr += sizeof(logger::severity);

		size_t stringLength = *reinterpret_cast<const size_t*>(ptr);
		ptr += sizeof(size_t);
		std::string string(ptr, stringLength);

		logger->log(string, severity);

		connection->sendMessage(SharedObject(thisStatusCode,
				SharedObject::RequestResponseCode::OK, SharedObject::NULL_DATA));
	}
};


#endif //PROGC_LOG_SERVER_PROCESSOR_H
