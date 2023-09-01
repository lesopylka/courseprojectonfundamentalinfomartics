#ifndef PROGC_SRC_DATA_TYPES_CONTEST_INFO_H
#define PROGC_SRC_DATA_TYPES_CONTEST_INFO_H


#include <string>
#include "../extensions/hashable.h"
#include "../extensions/serializable.h"
#include "../string_pool/string_pool.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>


class ContestInfo : public Hashable, public Serializable
{
private:

	int candidate_id;
	const std::string* last_name;
	const std::string* first_name;
	const std::string* patronymic;
	const std::string* birth_date;
	const std::string* resume_link;
	int hr_manager_id;
	int contest_id;
	const std::string* programming_language;
	int num_tasks;
	int solved_tasks;
	bool cheating_detected;

	static inline StringPool& string_pool = StringPool::instance();

public:

	ContestInfo(int candidate_id,
		const std::string& last_name,
		const std::string& first_name,
		const std::string& patronymic,
		const std::string& birth_date,
		const std::string& resume_link,
		int hr_manager_id,
		int contest_id,
		const std::string& programming_language,
		int num_tasks,
		int solved_tasks,
		bool cheating_detected)
		: candidate_id(candidate_id), last_name(&string_pool.get_string(last_name)),
		  first_name(&string_pool.get_string(first_name)), patronymic(&string_pool.get_string(patronymic)),
		  birth_date(&string_pool.get_string(birth_date)), resume_link(&string_pool.get_string(resume_link)),
		  hr_manager_id(hr_manager_id), contest_id(contest_id),
		  programming_language(&string_pool.get_string(programming_language)), num_tasks(num_tasks),
		  solved_tasks(solved_tasks), cheating_detected(cheating_detected)
	{
	}

	ContestInfo(const ContestInfo& other)
		: candidate_id(other.candidate_id),
		  last_name(&string_pool.get_string(*other.last_name)),
		  first_name(&string_pool.get_string(*other.first_name)),
		  patronymic(&string_pool.get_string(*other.patronymic)),
		  birth_date(&string_pool.get_string(*other.birth_date)),
		  resume_link(&string_pool.get_string(*other.resume_link)),
		  hr_manager_id(other.hr_manager_id),
		  contest_id(other.contest_id),
		  programming_language(&string_pool.get_string(*other.programming_language)),
		  num_tasks(other.num_tasks),
		  solved_tasks(other.solved_tasks),
		  cheating_detected(other.cheating_detected)
	{
	}

	static ContestInfo get_obj_for_search(int candidate_id, int contest_id)
	{
		std::string null;
		return {candidate_id, null, null, null, null, null, 0, contest_id, null, 0, 0, false};
	}

	size_t hashcode() const override
	{
		size_t h1 = std::hash<int>()(candidate_id);
		size_t h2 = std::hash<int>()(contest_id);
		return h1 ^ (h2 << 1);
	}

	bool operator==(const ContestInfo& other) const
	{
		return this->candidate_id == other.candidate_id && this->contest_id == other.contest_id;
	}

	ContestInfo& operator=(const ContestInfo& other)
	{
		if (this != &other)
		{
			last_name = &string_pool.get_string(other.getLastName());
			first_name = &string_pool.get_string(other.getFirstName());
			patronymic = &string_pool.get_string(other.getPatronymic());
			birth_date = &string_pool.get_string(other.getBirthDate());
			resume_link = &string_pool.get_string(other.getResumeLink());
			programming_language = &string_pool.get_string(other.getProgrammingLanguage());

			candidate_id = other.getCandidateId();
			hr_manager_id = other.getHrManagerId();
			contest_id = other.getContestId();
			num_tasks = other.getNumTasks();
			solved_tasks = other.getSolvedTasks();
			cheating_detected = other.isCheatingDetected();
		}
		return *this;
	}

	std::string serialize() const override {
		std::stringstream os;
		boost::archive::text_oarchive oa(os);
		oa << candidate_id;
		oa << *last_name;
		oa << *first_name;
		oa << *patronymic;
		oa << *birth_date;
		oa << *resume_link;
		oa << hr_manager_id;
		oa << contest_id;
		oa << *programming_language;
		oa << num_tasks;
		oa << solved_tasks;
		oa << cheating_detected;
		return os.str();
	}

	static ContestInfo deserialize(const std::string& serializedContestInfo) {
		std::stringstream is(serializedContestInfo);
		boost::archive::text_iarchive ia(is);

		int candidate_id;
		std::string last_name;
		std::string first_name;
		std::string patronymic;
		std::string birth_date;
		std::string resume_link;
		int hr_manager_id;
		int contest_id;
		std::string programming_language;
		int num_tasks;
		int solved_tasks;
		bool cheating_detected;

		ia >> candidate_id;
		ia >> last_name;
		ia >> first_name;
		ia >> patronymic;
		ia >> birth_date;
		ia >> resume_link;
		ia >> hr_manager_id;
		ia >> contest_id;
		ia >> programming_language;
		ia >> num_tasks;
		ia >> solved_tasks;
		ia >> cheating_detected;

		return { candidate_id,
			last_name,
			first_name,
			patronymic,
			birth_date,
			resume_link,
			hr_manager_id,
			contest_id,
			programming_language,
			num_tasks,
			solved_tasks,
			cheating_detected };
	}

	int getCandidateId() const
	{
		return candidate_id;
	}

	const std::string& getLastName() const
	{
		return *last_name;
	}

	const std::string& getFirstName() const
	{
		return *first_name;
	}

	const std::string& getPatronymic() const
	{
		return *patronymic;
	}

	const std::string& getBirthDate() const
	{
		return *birth_date;
	}

	const std::string& getResumeLink() const
	{
		return *resume_link;
	}

	int getHrManagerId() const
	{
		return hr_manager_id;
	}

	int getContestId() const
	{
		return contest_id;
	}

	const std::string& getProgrammingLanguage() const
	{
		return *programming_language;
	}

	int getNumTasks() const
	{
		return num_tasks;
	}

	int getSolvedTasks() const
	{
		return solved_tasks;
	}

	bool isCheatingDetected() const
	{
		return cheating_detected;
	}

	void print() const {
		std::stringstream result;
		result << std::endl << "__________________________" << std::endl;
		result << "Candidate ID: " << std::to_string(candidate_id) << std::endl;
		result << "Last name: " << *last_name << std::endl;
		result << "First name: " << *first_name << std::endl;
		result << "Patronymic: " << *patronymic << std::endl;
		result << "Birth date: " << *birth_date << std::endl;
		result << "Resume link: " << *resume_link << std::endl;
		result << "HR manager ID: " << std::to_string(hr_manager_id) << std::endl;
		result << "Contest ID: " << std::to_string(contest_id) << std::endl;
		result << "Programming language: " << *programming_language << std::endl;
		result << "Number of tasks: " << std::to_string(num_tasks) << std::endl;
		result << "Number of solved tasks: " << std::to_string(solved_tasks) << std::endl;
		result << "Cheating detected: " << (cheating_detected ? "true" : "false") << std::endl;
		result << "============================" << std::endl;
		std::cout << result.str();
	}
};


#endif //PROGC_SRC_DATA_TYPES_CONTEST_INFO_H
