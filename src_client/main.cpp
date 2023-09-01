#include "processors/client/client_processor.h"
#include "loggers/server_logger/server_logger.h"


const std::string CON_MEM_NAME = "con_mem";
const std::string CON_MUTEX_NAME = "con_mutex";
const int CLIENT_STATUS_CODE = 2;
const int LOG_SERVER_STATUS_CODE = 4;
const std::string LOG_MEM_NAME = "log_mem";
const std::string LOG_MUTEX_NAME = "log_mutex";


int main()
{
	ServerLogger serverLogger(LOG_SERVER_STATUS_CODE, LOG_MEM_NAME, LOG_MUTEX_NAME);
	ClientProcessor clientProcessor(CLIENT_STATUS_CODE, CON_MEM_NAME, CON_MUTEX_NAME, serverLogger);
	clientProcessor.interactiveMenu();

	return 0;
}