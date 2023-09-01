#include "processors/server/server_processor.h"
#include "loggers/server_logger/server_logger.h"


const std::string CON_MEM_NAME = "con_mem";
const std::string CON_MUTEX_NAME = "con_mutex";
const int SERVER_STATUS_CODE = 1;
const int LOG_SERVER_STATUS_CODE = 4;
const std::string LOG_MEM_NAME = "log_mem";
const std::string LOG_MUTEX_NAME = "log_mutex";


int main()
{
	ServerLogger serverLogger(LOG_SERVER_STATUS_CODE, LOG_MEM_NAME, LOG_MUTEX_NAME);
	ServerProcessor serverProcessor(SERVER_STATUS_CODE, CON_MEM_NAME, CON_MUTEX_NAME, serverLogger);
	while (true)
	{
		serverProcessor.process();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	return 0;
}