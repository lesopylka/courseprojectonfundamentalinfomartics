#ifndef PROGC_SRC_STRING_POOL_STRING_POOL_H
#define PROGC_SRC_STRING_POOL_STRING_POOL_H


#include <iostream>
#include <string>
#include <unordered_map>


class StringPool
{
private:
	StringPool()
	{
	}

	std::unordered_map<std::string, int> string_pool_;

public:

	static StringPool& instance()
	{
		static StringPool pool;
		return pool;
	}

	const std::string& get_string(const std::string& str)
	{
		auto it = string_pool_.find(str);
		if (it != string_pool_.end())
		{
			it->second++;
			return it->first;
		}
		else
		{
			auto [new_it, _] = string_pool_.emplace(str, 1);
			return new_it->first;
		}


	}

	void unget_string(const std::string& str)
	{
		auto it = string_pool_.find(str);
		if (it != string_pool_.end())
		{
			it->second--;
			if (it->second <= 0)
				string_pool_.erase(it);
		}

	}

	~StringPool() {
		string_pool_.clear();
	}
};


#endif //PROGC_SRC_STRING_POOL_STRING_POOL_H
