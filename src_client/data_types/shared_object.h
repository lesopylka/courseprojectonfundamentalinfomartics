#ifndef PROGC_SRC_DATA_TYPES_SHARED_OBJECT_H
#define PROGC_SRC_DATA_TYPES_SHARED_OBJECT_H


#include <sstream>
#include <utility>
#include <optional>
#include "../extensions/serializable.h"
#include <iostream>


/*
 Все соединения ч/з разделяемую память односторонние, т.е. кто-то один только запрашивает,
 а другой только отвечает:
 клиенты -> сервер
 сервер -> хранилища
 клиенты, сервер, хранилища -> лог_сервер
 */


class SharedObject : public Serializable
{
public:

	enum RequestResponseCode
	{
		REQUEST = 10,
		LOG = 13,
		GET_CONNECTION_CLIENT = 14,
        GET_CONNECTION_STORAGE = 16,
		CLOSE_CONNECTION = 15,
		OK = 20,
		ERROR = 21,
		STORAGE_REBALANCE = 30,
	};

private:

	char status_code; // первый байт - обработались ли данные
	char request_response_code; // код запроса / ответа
	const std::string data; // "null" - NULL

public:

	static inline const std::string NULL_DATA = "null";

	SharedObject(int statusCode, RequestResponseCode requestResponseCode, const Serializable& data)
			: status_code(static_cast<char>(statusCode)),
			  request_response_code(static_cast<char>(requestResponseCode)), data(data.serialize())
	{
	}

	// data may be "null"
	SharedObject(int statusCode, int requestResponseCode, const std::string& data)
			: status_code(static_cast<char>(statusCode)),
			  request_response_code(static_cast<char>(requestResponseCode)), data(data)
	{
	}

	std::string serialize() const override
	{
		std::stringstream ss;
		ss << status_code;
		ss << request_response_code;
		size_t tmp = data.length();
		ss << std::string(reinterpret_cast<char*>(&tmp), sizeof(tmp)) << data;
		ss << data;
		return ss.str();
	}

	static SharedObject deserialize(const char* serializedSharedObject)
	{
		char status_code = *serializedSharedObject;
		serializedSharedObject++;
		char request_response_code = *serializedSharedObject;
		serializedSharedObject++;
		size_t dataLen = *reinterpret_cast<const size_t*>(serializedSharedObject);
		serializedSharedObject += sizeof(size_t);
		return { status_code, request_response_code, std::string(serializedSharedObject, dataLen) };
	}

	static int getStatusCode(const char* serializedSharedObject)
	{
		return *serializedSharedObject;
	}

	int getStatusCode() const
	{
		return status_code;
	}

	RequestResponseCode getRequestResponseCode() const
	{
		return static_cast<RequestResponseCode>(request_response_code);
	}

	std::optional<std::string> getData() const
	{
		if (data == NULL_DATA)
			return std::nullopt;
		return { data };
	}

	void setStatusCode(int statusCode)
	{
		status_code = static_cast<char>(statusCode);
	}

	void print()
	{
		std::cout << std::endl << "Status code: " << (int)status_code << std::endl << "ReqRes code: "
				  << (int)request_response_code << std::endl << "Data: " << data << std::endl;
	}

	std::string getPrint()
	{
		std::stringstream ss;
		ss << std::endl << "Status code: " << (int)status_code << std::endl << "ReqRes code: "
		   << (int)request_response_code << std::endl << "Data: " << data << std::endl;
		return ss.str();
	}
};


#endif //PROGC_SRC_DATA_TYPES_SHARED_OBJECT_H
