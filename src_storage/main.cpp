#include "processors/storage/storage_processor.h"
#include "loggers/server_logger/server_logger.h"


const std::string CON_MEM_NAME = "con_mem";
const std::string CON_MUTEX_NAME = "con_mutex";
const int STORAGE_STATUS_CODE = 3;
const int LOG_SERVER_STATUS_CODE = 4;
const std::string LOG_MEM_NAME = "log_mem";
const std::string LOG_MUTEX_NAME = "log_mutex";


int main()
{
	ServerLogger serverLogger(LOG_SERVER_STATUS_CODE, LOG_MEM_NAME, LOG_MUTEX_NAME);
	StorageProcessor storageProcessor(STORAGE_STATUS_CODE, CON_MEM_NAME, CON_MUTEX_NAME, serverLogger);
	while (true)
	{
		storageProcessor.process();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	return 0;
}