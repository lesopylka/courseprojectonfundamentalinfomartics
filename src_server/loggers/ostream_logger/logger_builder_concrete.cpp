#include <fstream>
#include <sstream>
#include "logger_builder_concrete.h"
#include "logger_concrete.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using boost::property_tree::ptree;


logger_builder* logger_builder_concrete::add_stream(
	std::string const& path,
	logger::severity severity)
{
	_construction_info[path] = severity;

	return this;
}

logger* logger_builder_concrete::construct() const
{
	return new logger_concrete(_construction_info);
}

logger* logger_builder_concrete::file_construct(const std::string& filename)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file: " + filename);
	}

	logger_builder* builder = new logger_builder_concrete();

	ptree root;
	read_json(filename, root);

	for (const auto& [path_str, severity_str] : root) {
		logger::severity severity = logger_concrete::severityFromString(severity_str.get_value<std::string>());
		builder->add_stream(path_str, severity);
	}

	auto logger = builder->construct();
	delete builder;
	return logger;
}
