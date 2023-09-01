#ifndef PROGC_REQUEST_OBJECT_H
#define PROGC_REQUEST_OBJECT_H


#include <string>
#include <sstream>
#include <boost/archive/text_oarchive.hpp>
#include "../extensions/serializable.h"
#include "../extensions/hashable.h"


template<typename T>
class RequestObject : public Serializable
{
	static_assert(std::is_base_of<Serializable, T>::value,
			"T must extend Serializable");
	static_assert(std::is_base_of<Hashable, T>::value,
			"T must extend Hashable");

public:

	enum RequestCode {
		ADD = 10,
		CONTAINS = 11,
		REMOVE = 12,
		GET_KEY = 13,
		DELETE_DATABASE = 14,
		DELETE_SCHEMA = 15,
		DELETE_TABLE = 16,
	};

private:

	const RequestCode requestCode;
	const std::string database;
	const std::string schema;
	const std::string table;
	const std::string data; // "null" - NULL

public:

	static inline const std::string NULL_DATA = "null";

	RequestObject(RequestCode requestCode, const T& data, const std::string& database,
			const std::string& schema, const std::string& table)
			: requestCode(requestCode), data(data.serialize()), database(database), schema(schema), table(table)
	{
	}

	RequestObject(RequestCode requestCode, const std::string& data, const std::string& database,
			const std::string& schema, const std::string& table)
			: requestCode(requestCode), data(data), database(database), schema(schema), table(table)
	{
	}

	std::string serialize() const override
	{
		std::stringstream ss;
		ss << std::string(reinterpret_cast<const char * const>(&requestCode), sizeof(requestCode));
		size_t tmp = database.length();
		ss << std::string(reinterpret_cast<char *>(&tmp), sizeof(tmp)) << database;
		tmp = schema.length();
		ss << std::string(reinterpret_cast<char *>(&tmp), sizeof(tmp)) << schema;
		tmp = table.length();
		ss << std::string(reinterpret_cast<char *>(&tmp), sizeof(tmp)) << table;
		tmp = data.length();
		ss << std::string(reinterpret_cast<char *>(&tmp), sizeof(tmp)) << data;
		return ss.str();
	}

	static RequestObject deserialize(std::string& serializedRequestObject)
	{
		const char* ptr = serializedRequestObject.c_str();

		RequestCode requestCode = *reinterpret_cast<const RequestCode *>(ptr);
		ptr += sizeof(RequestCode);

		size_t databaseLength = *reinterpret_cast<const size_t *>(ptr);
		ptr += sizeof(size_t);
		std::string database(ptr, databaseLength);
		ptr += databaseLength;

		size_t schemaLength = *reinterpret_cast<const size_t *>(ptr);
		ptr += sizeof(size_t);
		std::string schema(ptr, schemaLength);
		ptr += schemaLength;

		size_t tableLength = *reinterpret_cast<const size_t *>(ptr);
		ptr += sizeof(size_t);
		std::string table(ptr, tableLength);
		ptr += tableLength;

		size_t dataLength = *reinterpret_cast<const size_t *>(ptr);
		ptr += sizeof(size_t);
		std::string data(ptr, dataLength);

		return { requestCode, data, database, schema, table };
	}

	const RequestCode getRequestCode() const
	{
		return requestCode;
	}

	const std::string& getDatabase() const
	{
		return database;
	}

	const std::string& getSchema() const
	{
		return schema;
	}

	const std::string& getTable() const
	{
		return table;
	}

	const std::string& getData() const
	{
		return data;
	}
};


#endif //PROGC_REQUEST_OBJECT_H
