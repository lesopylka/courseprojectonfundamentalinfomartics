#ifndef SANDBOX_CPP_LOGGER_CONCRETE_H
#define SANDBOX_CPP_LOGGER_CONCRETE_H

#include "../logger.h"
#include "logger_builder_concrete.h"
#include <map>

class logger_concrete final : public logger
{

	friend class logger_builder_concrete;

private:

	std::map<std::string, std::pair<std::ofstream*, logger::severity> > _logger_streams;

private:

	static std::map<std::string, std::pair<std::ofstream*, size_t> > _streams;

	static const std::map<severity, std::string> severity_string;

	static severity severityFromString(const std::string& severity_str);

private:

	explicit logger_concrete(std::map<std::string, logger::severity> const&);

public:

	logger_concrete(logger_concrete const&) = delete;

	logger_concrete& operator=(logger_concrete const&) = delete;

    logger_concrete(logger_concrete&&) = delete;

    logger_concrete& operator=(logger_concrete&&) = delete;

	~logger_concrete() override;

public:

	logger const* log(const std::string&, severity) const override;

};

#endif //SANDBOX_CPP_LOGGER_CONCRETE_H
