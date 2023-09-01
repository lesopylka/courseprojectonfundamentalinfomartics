#ifndef PROGC_SRC_PROCESSORS_CLIENT_CLIENT_PROCESSOR_H
#define PROGC_SRC_PROCESSORS_CLIENT_CLIENT_PROCESSOR_H


#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <thread>
#include <random>
#include <fstream>
#include "../../connection/connection.h"
#include "../../connection/memory_connection.h"
#include "../processor.h"
#include "../../data_types/shared_object.h"
#include "../../collections/Map.h"
#include "../../data_types/contest_info.h"
#include "../../data_types/request_object.h"
#include "../../loggers/server_logger/server_logger.h"


using namespace boost::interprocess;


class ClientProcessor : public Processor
{
private:

	const int thisStatusCode;
	const Connection* connection;
	std::string connectionName;
	ServerLogger& logger;

	void waitResponse()
	{
		while (SharedObject::getStatusCode(connection->receiveMessage()) == thisStatusCode)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}

public:

	ClientProcessor(const int statusCode, const std::string& memNameForConnect,
			const std::string& mutexNameForConnect, ServerLogger& serverLogger)
			: thisStatusCode(statusCode), logger(serverLogger)
	{
		MemoryConnection connect_connection(false, memNameForConnect);
		named_mutex mutex(open_only, mutexNameForConnect.c_str());
		scoped_lock<named_mutex> lock(mutex);
		connect_connection.sendMessage(SharedObject(thisStatusCode,
                                                    SharedObject::RequestResponseCode::GET_CONNECTION_CLIENT, SharedObject::NULL_DATA));
		while (SharedObject::getStatusCode(connect_connection.receiveMessage()) == thisStatusCode)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		auto data = SharedObject::deserialize(connect_connection.receiveMessage());
		auto memName = data.getData();
		if (!memName)
			throw std::runtime_error("Unable to establish a connection");
		connection = new MemoryConnection(false, memName.value());

		connectionName = memName.value();
		std::stringstream log;
		log << "[CLIENT] Get connection: " << connectionName << std::endl;
		logger.logSync(log.str(), logger::severity::debug);
		std::cout << log.str();
	}

	~ClientProcessor() override
	{
		connection->sendMessage(SharedObject(thisStatusCode,
				SharedObject::RequestResponseCode::CLOSE_CONNECTION, SharedObject::NULL_DATA));
		delete connection;
	}

	bool add(const std::string& database, const std::string& schema, const std::string& table,
			const ContestInfo& value)
	{
		RequestObject<ContestInfo> request(RequestObject<ContestInfo>::RequestCode::ADD,
				value, database, schema, table);
		connection->sendMessage(SharedObject(thisStatusCode,
				SharedObject::RequestResponseCode::REQUEST, request));
		waitResponse();
		auto response = SharedObject::deserialize(connection->receiveMessage());
		if (response.getRequestResponseCode() != SharedObject::RequestResponseCode::OK)
			return false;
		std::string result = response.getData().value();
		if (result == "true")
			return true;
		return false;
	};

	std::optional<ContestInfo> get(const std::string& database, const std::string& schema,
			const std::string& table, const ContestInfo& value)
	{
		RequestObject<ContestInfo> request(RequestObject<ContestInfo>::RequestCode::GET_KEY,
				value, database, schema, table);
		connection->sendMessage(SharedObject(thisStatusCode,
				SharedObject::RequestResponseCode::REQUEST, request));
		waitResponse();
		auto response = SharedObject::deserialize(connection->receiveMessage());
		if (response.getRequestResponseCode() != SharedObject::RequestResponseCode::OK)
			return std::nullopt;
		auto data = response.getData();
		if (!data)
			return std::nullopt;
		return ContestInfo::deserialize(data.value());
	};

	bool contains(const std::string& database, const std::string& schema, const std::string& table,
			const ContestInfo& value)
	{
		RequestObject<ContestInfo> request(RequestObject<ContestInfo>::RequestCode::CONTAINS,
				value, database, schema, table);
		connection->sendMessage(SharedObject(thisStatusCode,
				SharedObject::RequestResponseCode::REQUEST, request));
		waitResponse();
		auto response = SharedObject::deserialize(connection->receiveMessage());
		if (response.getRequestResponseCode() != SharedObject::RequestResponseCode::OK)
			return false;
		std::string result = response.getData().value();
		if (result == "true")
			return true;
		return false;
	};

	bool remove(const std::string& database, const std::string& schema, const std::string& table,
			const ContestInfo& value)
	{
		RequestObject<ContestInfo> request(RequestObject<ContestInfo>::RequestCode::REMOVE,
				value, database, schema, table);
		connection->sendMessage(SharedObject(thisStatusCode,
				SharedObject::RequestResponseCode::REQUEST, request));
		waitResponse();
		auto response = SharedObject::deserialize(connection->receiveMessage());
		if (response.getRequestResponseCode() != SharedObject::RequestResponseCode::OK)
			return false;
		std::string result = response.getData().value();
		if (result == "true")
			return true;
		return false;
	};

	bool removeDatabase(const std::string& database)
	{
		RequestObject<ContestInfo> request(RequestObject<ContestInfo>::RequestCode::DELETE_DATABASE,
				RequestObject<ContestInfo>::NULL_DATA, database, RequestObject<ContestInfo>::NULL_DATA,
				RequestObject<ContestInfo>::NULL_DATA);
		connection->sendMessage(SharedObject(thisStatusCode,
				SharedObject::RequestResponseCode::REQUEST, request));
		waitResponse();
		auto response = SharedObject::deserialize(connection->receiveMessage());
		if (response.getRequestResponseCode() != SharedObject::RequestResponseCode::OK)
			return false;
		std::string result = response.getData().value();
		if (result == "true")
			return true;
		return false;
	};

	bool removeSchema(const std::string& database, const std::string& schema)
	{
		RequestObject<ContestInfo> request(RequestObject<ContestInfo>::RequestCode::DELETE_SCHEMA,
				RequestObject<ContestInfo>::NULL_DATA, database, schema,
				RequestObject<ContestInfo>::NULL_DATA);
		connection->sendMessage(SharedObject(thisStatusCode,
				SharedObject::RequestResponseCode::REQUEST, request));
		waitResponse();
		auto response = SharedObject::deserialize(connection->receiveMessage());
		if (response.getRequestResponseCode() != SharedObject::RequestResponseCode::OK)
			return false;
		std::string result = response.getData().value();
		if (result == "true")
			return true;
		return false;
	};

	bool removeTable(const std::string& database, const std::string& schema, const std::string& table)
	{
		RequestObject<ContestInfo> request(RequestObject<ContestInfo>::RequestCode::DELETE_TABLE,
				RequestObject<ContestInfo>::NULL_DATA, database, schema, table);
		connection->sendMessage(SharedObject(thisStatusCode,
				SharedObject::RequestResponseCode::REQUEST, request));
		waitResponse();
		auto response = SharedObject::deserialize(connection->receiveMessage());
		if (response.getRequestResponseCode() != SharedObject::RequestResponseCode::OK)
			return false;
		std::string result = response.getData().value();
		if (result == "true")
			return true;
		return false;
	};

	void log(const std::string& message, logger::severity severity)
	{
		logger.logSync(message, severity);
	};

	void process() override
	{
	};

private:

	std::string generateRandomContestInfoString()
	{
		// Инициализация генератора случайных чисел
		std::random_device rd;
		std::mt19937 gen(rd());

		// Диапазоны значений для каждого поля
		std::uniform_int_distribution<int> candidateIdDist(1000, 9999);
		std::uniform_int_distribution<int> hrManagerIdDist(1, 10);
		std::uniform_int_distribution<int> contestIdDist(1000, 9999);
		std::uniform_int_distribution<int> numTasksDist(1, 20);
		std::uniform_int_distribution<int> solvedTasksDist(0, 20);
		std::uniform_int_distribution<int> cheatingDetectedDist(0, 1);

		// Списки возможных значений для строковых полей
		std::vector<std::string> lastNames = { "Smith", "Johnson", "Williams", "Jones", "Brown" };
		std::vector<std::string> firstNames = { "John", "David", "Michael", "James", "Robert" };
		std::vector<std::string> patronymics = { "Lee", "Thomas", "Daniel", "Joseph", "William" };
		std::vector<std::string> programmingLanguages = { "C++", "Java", "Python", "JavaScript", "Ruby" };

		// Генерация случайных значений для каждого поля
		int candidateId = candidateIdDist(gen);
		std::string lastName = lastNames[std::uniform_int_distribution<size_t>(0, lastNames.size() - 1)(gen)];
		std::string firstName = firstNames[std::uniform_int_distribution<size_t>(0, firstNames.size() - 1)(gen)];
		std::string patronymic = patronymics[std::uniform_int_distribution<size_t>(0, patronymics.size() - 1)(gen)];
		std::string birthDate = "1990-01-01";  // Здесь можно использовать другую логику для генерации даты рождения
		std::string resumeLink = "resume_link";  // Здесь можно использовать другую логику для генерации ссылки на резюме
		int hrManagerId = hrManagerIdDist(gen);
		int contestId = contestIdDist(gen);
		std::string programmingLanguage = programmingLanguages[std::uniform_int_distribution<size_t>(0,
				programmingLanguages.size() - 1)(gen)];
		int numTasks = numTasksDist(gen);
		int solvedTasks = solvedTasksDist(gen);
		bool cheatingDetected = (cheatingDetectedDist(gen) == 1);

		// Создание строки с данными
		std::ostringstream oss;
		oss << candidateId << " " << lastName << " " << firstName << " " << patronymic << " " << birthDate << " "
			<< resumeLink << " " << hrManagerId << " " << contestId << " " << programmingLanguage << " " << numTasks
			<< " " << solvedTasks << " " << (cheatingDetected ? "true" : "false");
		return oss.str();
	}

	ContestInfo readContestInfoFromString(const std::string& contestInfoStr)
	{
		std::istringstream iss(contestInfoStr);
		std::vector<std::string> contestInfoTokens;

		std::string token;
		while (iss >> token)
		{
			contestInfoTokens.push_back(token);
		}

		if (contestInfoTokens.size() != 12)
		{
			throw std::runtime_error("Invalid contest info string format.");
		}

		int candidate_id = std::stoi(contestInfoTokens[0]);
		std::string last_name = contestInfoTokens[1];
		std::string first_name = contestInfoTokens[2];
		std::string patronymic = contestInfoTokens[3];
		std::string birth_date = contestInfoTokens[4];
		std::string resume_link = contestInfoTokens[5];
		int hr_manager_id = std::stoi(contestInfoTokens[6]);
		int contest_id = std::stoi(contestInfoTokens[7]);
		std::string programming_language = contestInfoTokens[8];
		int num_tasks = std::stoi(contestInfoTokens[9]);
		int solved_tasks = std::stoi(contestInfoTokens[10]);
		bool cheating_detected = (contestInfoTokens[11] == "true");

		return ContestInfo(candidate_id, last_name, first_name, patronymic, birth_date,
				resume_link, hr_manager_id, contest_id, programming_language,
				num_tasks, solved_tasks, cheating_detected);
	}

	ContestInfo readContestInfoFromCin()
	{
		std::cout << "Enter contest info string: ";
		std::string contestInfoStr;
		std::getline(std::cin, contestInfoStr);

		return readContestInfoFromString(contestInfoStr);
	}

	int readIntFromCin()
	{
		std::string input;
		int num;
		std::getline(std::cin, input);
		std::stringstream ss(input);
		ss >> num;
		return num;
	}

public:

	void interactiveMenu()
	{
		int choice;
		std::string database, schema, table, file_path;
		std::optional<ContestInfo> result;
		std::string input;

		while (true)
		{
			std::cout << "=== Interactive Menu ===" << std::endl;
			std::cout << "1. Add Contest" << std::endl;
			std::cout << "2. Get Contest" << std::endl;
			std::cout << "3. Check Contest Existence" << std::endl;
			std::cout << "4. Remove Contest" << std::endl;
			std::cout << "5. Remove Database" << std::endl;
			std::cout << "6. Remove Schema" << std::endl;
			std::cout << "7. Remove Table" << std::endl;
			std::cout << "8. Generate random contest info" << std::endl;
			std::cout << "9. File commands format" << std::endl;
			std::cout << "10. Read commands from file" << std::endl;
			std::cout << "11. Exit" << std::endl;
			std::cout << "Enter your choice: ";

			choice = readIntFromCin();

			switch (choice)
			{
			case 1:
				std::cout << "Enter database name: ";
				std::cin >> database;
				std::cout << "Enter schema name: ";
				std::cin >> schema;
				std::cout << "Enter table name: ";
				std::cin >> table;
				std::getchar();

				if (add(database, schema, table, readContestInfoFromCin()))
				{
					std::cout << "Contest added successfully." << std::endl;
				}
				else
				{
					std::cout << "Failed to add contest." << std::endl;
				}
				break;
			case 2:
				std::cout << "Enter database name: ";
				std::cin >> database;
				std::cout << "Enter schema name: ";
				std::cin >> schema;
				std::cout << "Enter table name: ";
				std::cin >> table;
				int candidateId;
				int contestId;
				std::getchar();
				std::cout << "Enter candidate ID: ";
				candidateId = readIntFromCin();
				std::cout << "Enter contest ID: ";
				contestId = readIntFromCin();

				result = get(database, schema, table,
						ContestInfo::get_obj_for_search(candidateId, contestId));
				if (result)
				{
					std::cout << "Found Contest: ";
					result.value().print();
				}
				else
				{
					std::cout << "Contest not found." << std::endl;
				}
				break;
			case 3:
				std::cout << "Enter database name: ";
				std::cin >> database;
				std::cout << "Enter schema name: ";
				std::cin >> schema;
				std::cout << "Enter table name: ";
				std::cin >> table;
				std::getchar();

				if (contains(database, schema, table, readContestInfoFromCin()))
				{
					std::cout << "Contest exists." << std::endl;
				}
				else
				{
					std::cout << "Contest does not exist." << std::endl;
				}
				break;
			case 4:
				std::cout << "Enter database name: ";
				std::cin >> database;
				std::cout << "Enter schema name: ";
				std::cin >> schema;
				std::cout << "Enter table name: ";
				std::cin >> table;
				std::getchar();

				if (remove(database, schema, table, readContestInfoFromCin()))
				{
					std::cout << "Contest removed successfully." << std::endl;
				}
				else
				{
					std::cout << "Failed to remove contest." << std::endl;
				}
				break;
			case 5:
				std::cout << "Enter database name: ";
				std::cin >> database;
				std::getchar();
				if (removeDatabase(database))
				{
					std::cout << "Database removed successfully." << std::endl;
				}
				else
				{
					std::cout << "Failed to remove database." << std::endl;
				}
				break;
			case 6:
				std::cout << "Enter database name: ";
				std::cin >> database;
				std::cout << "Enter schema name: ";
				std::cin >> schema;
				std::getchar();

				if (removeSchema(database, schema))
				{
					std::cout << "Schema removed successfully." << std::endl;
				}
				else
				{
					std::cout << "Failed to remove schema." << std::endl;
				}
				break;
			case 7:
				std::cout << "Enter database name: ";
				std::cin >> database;
				std::cout << "Enter schema name: ";
				std::cin >> schema;
				std::cout << "Enter table name: ";
				std::cin >> table;
				std::getchar();

				if (removeTable(database, schema, table))
				{
					std::cout << "Table removed successfully." << std::endl;
				}
				else
				{
					std::cout << "Failed to remove table." << std::endl;
				}
				break;
			case 8:
				std::cout << generateRandomContestInfoString() << std::endl;
				break;
			case 9:
				std::cout << "File commands format:" << std::endl;
				std::cout << "ADD;DATABASE;SCHEMA;TABLE;CONTEST_INFO" << std::endl;
				std::cout << "GET;DATABASE;SCHEMA;TABLE;CANDIDATE_ID;CONTEST_ID" << std::endl;
				std::cout << "CONTAINS;DATABASE;SCHEMA;TABLE;CONTEST_INFO" << std::endl;
				std::cout << "REMOVE;DATABASE;SCHEMA;TABLE;CONTEST_INFO" << std::endl;
				std::cout << "REMOVE_DATABASE;DATABASE" << std::endl;
				std::cout << "REMOVE_SCHEMA;DATABASE;SCHEMA" << std::endl;
				std::cout << "REMOVE_TABLE;DATABASE;SCHEMA;TABLE" << std::endl << std::endl;
				break;
			case 10:
				std::cout << "Enter file path: " << std::endl;
				std::cin >> file_path;
				std::getchar();
				fileCommands(file_path);
				break;
			case 11:
				std::cout << "Exiting..." << std::endl;
				return;
			default:
				std::cout << "Invalid choice. Please try again." << std::endl;
				break;
			}
		}
	}

	void fileCommands(const std::string& filename)
	{

		std::vector<std::vector<std::string>> commands;

		std::ifstream file(filename);
		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file: " + filename);
		}

		std::string line;
		while (std::getline(file, line)) {
			std::vector<std::string> command;
			std::string value;
			std::stringstream ss(line);

			while (std::getline(ss, value, ';')) {
				command.push_back(value);
			}

			commands.push_back(command);
		}

		file.close();

		for (const auto& command : commands) {
			if (command.empty()) {
				continue;
			}

			std::string cmd = command[0];

			if (cmd == "ADD") {
				// Обработка команды ADD
				// command[1] - DATABASE
				// command[2] - SCHEMA
				// command[3] - TABLE
				// command[4] - CONTEST_INFO
				if (command.size() != 5)
					throw std::runtime_error("Incorrect format");
				if (add(command[1], command[2], command[3], readContestInfoFromString(command[4])))
				{
					std::cout << "Contest added successfully." << std::endl;
				}
				else
				{
					std::cout << "Failed to add contest." << std::endl;
				}
			} else if (cmd == "GET") {
				// Обработка команды GET
				// command[1] - DATABASE
				// command[2] - SCHEMA
				// command[3] - TABLE
				// command[4] - CANDIDATE_ID
				// command[5] - CONTEST_ID
				if (command.size() != 6)
					throw std::runtime_error("Incorrect format");
				auto result = get(command[1], command[2], command[3],
						ContestInfo::get_obj_for_search(std::stoi(command[4]), std::stoi(command[5])));
				if (result)
				{
					std::cout << "Found Contest: ";
					result.value().print();
				}
				else
				{
					std::cout << "Contest not found." << std::endl;
				}
			} else if (cmd == "CONTAINS") {
				// Обработка команды CONTAINS
				// command[1] - DATABASE
				// command[2] - SCHEMA
				// command[3] - TABLE
				// command[4] - CONTEST_INFO
				if (command.size() != 5)
					throw std::runtime_error("Incorrect format");
				if (contains(command[1], command[2], command[3],
						readContestInfoFromString(command[4])))
				{
					std::cout << "Contest exists." << std::endl;
				}
				else
				{
					std::cout << "Contest does not exist." << std::endl;
				}
			} else if (cmd == "REMOVE") {
				// Обработка команды REMOVE
				// command[1] - DATABASE
				// command[2] - SCHEMA
				// command[3] - TABLE
				// command[4] - CONTEST_INFO
				if (command.size() != 5)
					throw std::runtime_error("Incorrect format");
				if (remove(command[1], command[2], command[3],
						readContestInfoFromString(command[4])))
				{
					std::cout << "Contest removed successfully." << std::endl;
				}
				else
				{
					std::cout << "Failed to remove contest." << std::endl;
				}
			} else if (cmd == "REMOVE_DATABASE") {
				// Обработка команды REMOVE_DATABASE
				// command[1] - DATABASE
				if (command.size() != 2)
					throw std::runtime_error("Incorrect format");
				if (removeDatabase(command[1]))
				{
					std::cout << "Database removed successfully." << std::endl;
				}
				else
				{
					std::cout << "Failed to remove database." << std::endl;
				}
			} else if (cmd == "REMOVE_SCHEMA") {
				// Обработка команды REMOVE_SCHEMA
				// command[1] - DATABASE
				// command[2] - SCHEMA
				if (command.size() != 3)
					throw std::runtime_error("Incorrect format");
				if (removeSchema(command[1], command[2]))
				{
					std::cout << "Schema removed successfully." << std::endl;
				}
				else
				{
					std::cout << "Failed to remove schema." << std::endl;
				}
			} else if (cmd == "REMOVE_TABLE") {
				// Обработка команды REMOVE_TABLE
				// command[1] - DATABASE
				// command[2] - SCHEMA
				// command[3] - TABLE
				if (command.size() != 4)
					throw std::runtime_error("Incorrect format");
				if (removeTable(command[1], command[2], command[3]))
				{
					std::cout << "Table removed successfully." << std::endl;
				}
				else
				{
					std::cout << "Failed to remove table." << std::endl;
				}
			} else {
				std::cout << "Invalid command: " << cmd << std::endl;
			}
		}

	}
};


#endif //PROGC_SRC_PROCESSORS_CLIENT_CLIENT_PROCESSOR_H
