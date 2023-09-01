#include <thread>
#include "loggers/ostream_logger/logger_builder_concrete.h"
#include "processors/log_server/log_server_processor.h"


const int LOG_SERVER_STATUS_CODE = 4;
const std::string LOG_MEM_NAME = "log_mem";
const std::string LOG_MUTEX_NAME = "log_mutex";


int main()
{
	logger* logger = logger_builder_concrete::file_construct("log_settings.txt");
	LogServerProcessor logServerProcessor(LOG_SERVER_STATUS_CODE, LOG_MEM_NAME, LOG_MUTEX_NAME, logger);
	while (true)
	{
		logServerProcessor.process();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	delete logger;


	return 0;
}