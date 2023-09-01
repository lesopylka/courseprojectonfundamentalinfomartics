

#include "data_types/contest_info.h"
#include "data_types/shared_object.h"
#include "processors/processor.h"
#include "processors/client/client_processor.h"
#include "processors/server/server_processor.h"
#include "processors/storage/storage_processor.h"
#include "loggers/ostream_logger/logger_builder.h"
#include "loggers/ostream_logger/logger_builder_concrete.h"
#include "loggers/server_logger/server_logger.h"
#include "processors/log_server/log_server_processor.h"


const std::string CON_MEM_NAME = "con_mem";
const std::string CON_MUTEX_NAME = "con_mutex";
const int SERVER_STATUS_CODE = 1;
const int CLIENT_STATUS_CODE = 2;
const int STORAGE_STATUS_CODE = 3;
const int LOG_SERVER_STATUS_CODE = 4;
const std::string LOG_MEM_NAME = "log_mem";
const std::string LOG_MUTEX_NAME = "log_mutex";


int main()
{
//	ServerLogger serverLogger(LOG_SERVER_STATUS_CODE, LOG_MEM_NAME, LOG_MUTEX_NAME);
//	ClientProcessor clientProcessor(CLIENT_STATUS_CODE, CON_MEM_NAME, CON_MUTEX_NAME, serverLogger);
//	clientProcessor.interactiveMenu();


//	ServerLogger serverLogger(LOG_SERVER_STATUS_CODE, LOG_MEM_NAME, LOG_MUTEX_NAME);
//	ServerProcessor serverProcessor(SERVER_STATUS_CODE, CON_MEM_NAME, CON_MUTEX_NAME, serverLogger);
//	while (true)
//	{
//		serverProcessor.process();
//		std::this_thread::sleep_for(std::chrono::seconds(1));
//	}


	ServerLogger serverLogger(LOG_SERVER_STATUS_CODE, LOG_MEM_NAME, LOG_MUTEX_NAME);
	StorageProcessor storageProcessor(STORAGE_STATUS_CODE, CON_MEM_NAME, CON_MUTEX_NAME, serverLogger);
	while (true)
	{
		storageProcessor.process();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}


//	logger* logger = logger_builder_concrete::file_construct("log_settings.txt");
//	LogServerProcessor logServerProcessor(LOG_SERVER_STATUS_CODE, LOG_MEM_NAME, LOG_MUTEX_NAME, logger);
//	while (true)
//	{
//		logServerProcessor.process();
//		std::this_thread::sleep_for(std::chrono::seconds(1));
//	}
//	delete logger;


	return 0;
}