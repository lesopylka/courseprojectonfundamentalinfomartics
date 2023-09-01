#include "logger_concrete.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip>

std::map<std::string, std::pair<std::ofstream*, size_t> > logger_concrete::_streams =
	std::map<std::string, std::pair<std::ofstream*, size_t> >();

const std::map<logger::severity, std::string> logger_concrete::severity_string = {
	{ severity::trace, "TRACE" },
	{ severity::debug, "DEBUG" },
	{ severity::information, "INFO" },
	{ severity::warning, "WARNING" },
	{ severity::error, "ERROR" },
	{ severity::critical, "CRITICAL" }
};

logger_concrete::logger_concrete(
	std::map<std::string, logger::severity> const& targets)
{
	for (auto& target : targets)
	{
		auto global_stream = _streams.find(target.first);
		std::ofstream* stream = nullptr;

		if (global_stream == _streams.end())
		{
			if (target.first != "console")
			{
				stream = new std::ofstream;
				stream->open(target.first);
			}

			_streams.insert(std::make_pair(target.first, std::make_pair(stream, 1)));
		}
		else
		{
			stream = global_stream->second.first;
			global_stream->second.second++;
		}

		_logger_streams.insert(std::make_pair(target.first, std::make_pair(stream, target.second)));
	}
}

logger_concrete::~logger_concrete()
{
	for (auto& logger_stream : _logger_streams)
	{
		auto global_stream = _streams.find(logger_stream.first);
		if (global_stream != _streams.end())
		{
			if (--(global_stream->second.second) == 0)
			{
				if (global_stream->second.first != nullptr)
				{
					global_stream->second.first->flush();
					global_stream->second.first->close();
					delete global_stream->second.first;
				}

				_streams.erase(global_stream);
			}
		}
	}
}

logger const* logger_concrete::log(
	const std::string& to_log,
	logger::severity severity) const
{
	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	std::stringstream time_ss;
	time_ss << std::put_time(&tm, "%d/%m/%Y %H:%M:%S");

	for (auto& logger_stream : _logger_streams)
	{
		if (logger_stream.second.second > severity)
		{
			continue;
		}

		if (logger_stream.second.first == nullptr)
		{
			std::cout << "[" << time_ss.str() << "][" << severity_string.at(severity) << "] " << to_log << std::endl;
		}
		else
		{
			(*logger_stream.second.first) << "[" << time_ss.str() << "][" << severity_string.at(severity) << "] "
										  << to_log << std::endl;
		}
	}

	return this;
}

logger::severity logger_concrete::severityFromString(const std::string& severity_str)
{
	for (const auto& [severity, str] : severity_string)
	{
		if (str == severity_str)
		{
			return severity;
		}
	}
	throw std::invalid_argument("Invalid severity string: " + severity_str);
}
