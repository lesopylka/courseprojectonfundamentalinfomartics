#ifndef PROGC_SRC_PROCESSORS_STORAGE_STORAGE_PROCESSOR_H
#define PROGC_SRC_PROCESSORS_STORAGE_STORAGE_PROCESSOR_H


#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <thread>
#include "../../connection/connection.h"
#include "../../connection/memory_connection.h"
#include "../processor.h"
#include "../../data_types/shared_object.h"
#include "../../data_types/contest_info.h"
#include "../../collections/Map.h"
#include "../../collections/BPlusTree/BPlusTreeMap.h"
#include "../../data_types/request_object.h"


using namespace boost::interprocess;


int stringComparer(const std::string& a, const std::string& b)
{
	return a.compare(b);
}

int contestInfoComparer(const ContestInfo& a, const ContestInfo& b)
{
	if (a.getContestId() < b.getContestId())
	{
		return -1;
	}
	else if (a.getContestId() > b.getContestId())
	{
		return 1;
	}
	if (a.getCandidateId() < b.getCandidateId())
	{
		return -1;
	}
	else if (a.getCandidateId() > b.getCandidateId())
	{
		return 1;
	}
	return 0;
}

class StorageProcessor : public Processor
{
private:

	BPlusTreeMap<std::string, std::shared_ptr
			<
					BPlusTreeMap<std::string, std::shared_ptr
							<
									BPlusTreeMap<std::string, std::shared_ptr
											<
													BPlusTreeMap<ContestInfo, Null>
											>
									>
							>
					>
			>
	> db;
	const int this_status_code;
	const Connection* connection;
	std::string connectionName;
	ServerLogger& logger;

	int storage_id;
	const Connection* client_connection;
	std::queue<RequestObject<ContestInfo>> toSend;

public:

	StorageProcessor(const int statusCode, const std::string& memNameForConnect,
			const std::string& mutexNameForConnect, ServerLogger& serverLogger)
			: this_status_code(statusCode), db(3, 3, stringComparer), logger(serverLogger)
	{
		MemoryConnection connect_connection(false, memNameForConnect);
		named_mutex mutex(open_only, mutexNameForConnect.c_str());
		scoped_lock<named_mutex> lock(mutex);
		connect_connection.sendMessage(SharedObject(this_status_code,
				SharedObject::RequestResponseCode::GET_CONNECTION_STORAGE, SharedObject::NULL_DATA));
		while (SharedObject::getStatusCode(connect_connection.receiveMessage()) == this_status_code)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		auto data = SharedObject::deserialize(connect_connection.receiveMessage());
		auto memNameStorage = data.getData();
		if (!memNameStorage)
			throw std::runtime_error("Unable to establish a connection");
		connection = new MemoryConnection(false, memNameStorage.value());
		storage_id = std::stoi(memNameStorage->substr(7));

		connect_connection.sendMessage(SharedObject(this_status_code,
				SharedObject::RequestResponseCode::GET_CONNECTION_CLIENT, SharedObject::NULL_DATA));
		while (SharedObject::getStatusCode(connect_connection.receiveMessage()) == this_status_code)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		auto data1 = SharedObject::deserialize(connect_connection.receiveMessage());
		auto memNameClient = data1.getData();
		if (!memNameClient)
			throw std::runtime_error("Unable to establish a connection");
		client_connection = new MemoryConnection(false, memNameClient.value());

		connectionName = memNameStorage.value() + " | " + memNameClient.value();

		std::stringstream log;
		log << "[STORAGE] Get connection: " << connectionName << std::endl;
		logger.logSync(log.str(), logger::severity::debug);
		std::cout << log.str();
	}

	~StorageProcessor() override
	{
		delete connection;
		delete client_connection;
	}

	void process() override
	{
		logger.process();

		if ((SharedObject::getStatusCode(client_connection->receiveMessage()) != this_status_code))
		{
			if (!toSend.empty())
			{
				client_connection->sendMessage(SharedObject(this_status_code,
						SharedObject::RequestResponseCode::REQUEST, toSend.front()));
				toSend.pop();
			}
		}

		if ((SharedObject::getStatusCode(connection->receiveMessage()) != this_status_code))
		{
			SharedObject message = SharedObject::deserialize(connection->receiveMessage());

			std::stringstream log;
			log << "[" << connectionName << "] Receive:" << std::endl << message.getPrint();
			std::cout << log.str() << std::endl;
			logger.log(log.str(), logger::severity::debug);

			auto messageData = message.getData();

			if (message.getRequestResponseCode() == SharedObject::STORAGE_REBALANCE)
			{
				const char* ptr = messageData.value().c_str();
				size_t storage_count = *reinterpret_cast<const size_t*>(ptr);

				std::queue<RequestObject<ContestInfo>> toDelete;

				if (db.size() > 0)
				{
					auto dbIterator = db.begin();
					while (true)
					{
						auto dbPair = dbIterator.entry;
						std::string dbName = *(dbPair->key);
						auto schema = dbPair->value->get();
						if (schema->size() > 0)
						{
							auto schemaIterator = schema->begin();
							while (true)
							{
								auto schemaPair = schemaIterator.entry;
								std::string schemaName = *(schemaPair->key);
								auto table = schemaPair->value->get();
								if (table->size() > 0)
								{
									auto tableIterator = table->begin();
									while (true)
									{
										auto tablePair = tableIterator.entry;
										std::string tableName = *(tablePair->key);
										auto data = tablePair->value->get();
										if (data->size() > 0)
										{
											auto dataIterator = data->begin();
											while (true)
											{
												ContestInfo* contestInfo = dataIterator.entry->key;
												if (contestInfo->hashcode() % storage_count != storage_id)
												{
													toSend.emplace(RequestObject<ContestInfo>::RequestCode::ADD,
															contestInfo->serialize(), dbName, schemaName, tableName);
													toDelete.emplace(RequestObject<ContestInfo>::RequestCode::ADD,
															ContestInfo::get_obj_for_search(
																	contestInfo->getCandidateId(),
																	contestInfo->getContestId()), dbName, schemaName,
															tableName);

													std::cout << "Removed for rebalancing: " << contestInfo->serialize() << std::endl << std::endl;
												}

												if (dataIterator == data->end())
													break;
												dataIterator += 1;
											}
										}

										if (tableIterator == table->end())
											break;
										tableIterator += 1;
									}
								}
								if (schemaIterator == schema->end())
									break;
								schemaIterator += 1;
							}
						}
						if (dbIterator == db.end())
							break;
						dbIterator += 1;
					}
				}

				connection->sendMessage(SharedObject(this_status_code,
						SharedObject::RequestResponseCode::OK, SharedObject::NULL_DATA));

				while (!toDelete.empty())
				{
					auto request = toDelete.front();
					auto schemas = db.get(request.getDatabase());
					if (schemas)
					{
						auto tables = schemas.value()->get(request.getSchema());
						if (tables)
						{
							auto table = tables.value()->get(request.getTable());
							if (table)
							{
								table.value()->remove(ContestInfo::deserialize(request.getData()));
							}
						}
					}
					toDelete.pop();
				}
				return;
			}

			if (!messageData || message.getRequestResponseCode() != SharedObject::RequestResponseCode::REQUEST)
			{
				connection->sendMessage(SharedObject(this_status_code, SharedObject::RequestResponseCode::ERROR,
						SharedObject::NULL_DATA));
				return;
			}
			auto request = RequestObject<ContestInfo>::deserialize(messageData.value());
			std::string response = SharedObject::NULL_DATA;

			switch (request.getRequestCode())
			{
			case RequestObject<ContestInfo>::ADD:
			{
				ContestInfo data = ContestInfo::deserialize(request.getData());
				auto schemasOpt = db.get(request.getDatabase());
				if (!schemasOpt)
				{
					auto schemas = std::make_shared<BPlusTreeMap<std::string, std::shared_ptr<BPlusTreeMap<std::string,
							std::shared_ptr<BPlusTreeMap<ContestInfo, Null>>>>>>(3, 3, stringComparer);
					if (!db.add(request.getDatabase(), schemas))
						throw std::runtime_error("Can't add database");
					schemasOpt.emplace(schemas);
				}
				auto schemas = schemasOpt.value();

				auto tablesOpt = schemas->get(request.getSchema());
				if (!tablesOpt)
				{
					auto tables = std::make_shared<BPlusTreeMap<std::string,
							std::shared_ptr<BPlusTreeMap<ContestInfo, Null>>>>(3, 3, stringComparer);
					if (!schemas->add(request.getSchema(), tables))
						throw std::runtime_error("Can't add schema");
					tablesOpt.emplace(tables);
				}
				auto tables = tablesOpt.value();

				auto tableOpt = tables->get(request.getTable());
				if (!tableOpt)
				{
					auto table = std::make_shared<BPlusTreeMap<ContestInfo, Null>>(3, 3, contestInfoComparer);
					if (!tables->add(request.getTable(), table))
						throw std::runtime_error("Can't add table");
					tableOpt.emplace(table);
				}
				auto table = tableOpt.value();

				if (table->add(data, Null::value()))
					response = "true";
				else
					response = "false";
				break;
			}
			case RequestObject<ContestInfo>::CONTAINS:
			{
				ContestInfo data = ContestInfo::deserialize(request.getData());
				bool contains = false;
				auto schemas = db.get(request.getDatabase());
				if (schemas)
				{
					auto tables = schemas.value()->get(request.getSchema());
					if (tables)
					{
						auto table = tables.value()->get(request.getTable());
						if (table)
						{
							contains = table.value()->contains(data);
						}
					}
				}
				if (contains)
					response = "true";
				else
					response = "false";
				break;
			}
			case RequestObject<ContestInfo>::REMOVE:
			{
				ContestInfo data = ContestInfo::deserialize(request.getData());
				bool removed = false;
				auto schemas = db.get(request.getDatabase());
				if (schemas)
				{
					auto tables = schemas.value()->get(request.getSchema());
					if (tables)
					{
						auto table = tables.value()->get(request.getTable());
						if (table)
						{
							removed = table.value()->remove(data);
						}
					}
				}
				if (removed)
					response = "true";
				else
					response = "false";
				break;
			}
			case RequestObject<ContestInfo>::GET_KEY:
			{
				ContestInfo data = ContestInfo::deserialize(request.getData());
				auto schemas = db.get(request.getDatabase());
				if (schemas)
				{
					auto tables = schemas.value()->get(request.getSchema());
					if (tables)
					{
						auto table = tables.value()->get(request.getTable());
						if (table)
						{
							auto listVal = table.value()->entrySet(data, data);
							if (listVal.size() == 1)
							{
								response = listVal.at(0).getKey().serialize();
							}
						}
					}
				}
				break;
			}
			case RequestObject<ContestInfo>::DELETE_DATABASE:
			{
				bool removed = db.remove(request.getDatabase());
				if (!removed)
				{
					connection->sendMessage(SharedObject(this_status_code,
							SharedObject::RequestResponseCode::ERROR, response));
					return;
				}
				break;
			}
			case RequestObject<ContestInfo>::DELETE_SCHEMA:
			{
				bool removed = false;
				auto schemas = db.get(request.getDatabase());
				if (schemas)
				{
					removed = schemas.value()->remove(request.getSchema());
				}
				if (!removed)
				{
					connection->sendMessage(SharedObject(this_status_code,
							SharedObject::RequestResponseCode::ERROR, response));
					return;
				}
				break;
			}
			case RequestObject<ContestInfo>::DELETE_TABLE:
			{
				bool removed = false;
				auto schemas = db.get(request.getDatabase());
				if (schemas)
				{
					auto tables = schemas.value()->get(request.getSchema());
					if (tables)
					{
						removed = tables.value()->remove(request.getTable());
					}
				}
				if (!removed)
				{
					connection->sendMessage(SharedObject(this_status_code,
							SharedObject::RequestResponseCode::ERROR, response));
					return;
				}
			}
			default:
			{
				connection->sendMessage(SharedObject(this_status_code,
						SharedObject::RequestResponseCode::ERROR, response));
			}
			}
			connection->sendMessage(SharedObject(this_status_code,
					SharedObject::RequestResponseCode::OK, response));
		}
	}
};


#endif //PROGC_SRC_PROCESSORS_STORAGE_STORAGE_PROCESSOR_H
