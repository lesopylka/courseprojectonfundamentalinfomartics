#ifndef PROGC_SRC_BPLUSTREE_ALLOCATORS_DEFAULT_MEMORY_H
#define PROGC_SRC_BPLUSTREE_ALLOCATORS_DEFAULT_MEMORY_H


#include <new>
#include "memory.h"
#include <sstream>
#include "../../loggers/logger.h"

// allocators
#define DEFAULT_ALLOC 0
#define SORTED_LIST_ALLOC 1
#define BORDER_DESCRIPTOR_ALLOC 2
#define BODY_ALLOC 3

// methods
#define FIRST first
#define BEST best
#define WORST worst


#define ALLOC_SIZE 1000000
#define ALLOC_METHOD FIRST
#define CURRENT_ALLOC SORTED_LIST_ALLOC


#ifdef CURRENT_ALLOC
#if CURRENT_ALLOC == DEFAULT_ALLOC

class DefaultMemory : public Memory
{

public:

	void* allocate(size_t target_size) const override
	{
		void* mem;
		try
		{
			mem = ::operator new(target_size);
		}
		catch (std::bad_alloc& e)
		{
			throw e;
		}
		return mem;
	}

	void deallocate(void* const target_to_dealloc) const override
	{
		::operator delete(target_to_dealloc);
	}

	DefaultMemory() = default;

	DefaultMemory(DefaultMemory const&) = delete;

	DefaultMemory& operator=(DefaultMemory const&) = delete;
};

#elif CURRENT_ALLOC == SORTED_LIST_ALLOC

class DefaultMemory : public Memory
{
public:

	enum allocation_method
	{
		first, best, worst
	};

private:
	// available block: size_t | next_ptr
	// occupied block: size_t

	// data: method | alloc_ptr | logger_ptr | first_available_ptr | ...
	char* data;

	bool has_logger() const
	{
		char* current = data;
		current += (sizeof(allocation_method) + sizeof(Memory*));
		logger* log_ = *reinterpret_cast<logger**>(current);
		return log_ != nullptr;
	};

	void log(std::string const& str, logger::severity severity) const
	{
		char* current = data;
		current += (sizeof(allocation_method) + sizeof(Memory*));
		logger* log_ = *reinterpret_cast<logger**>(current);
		if (log_ == nullptr)
			return;
		log_->log(str, severity);
	}

	size_t get_service_block_size() const
	{
		return sizeof(allocation_method) + sizeof(Memory*) + sizeof(logger*) + sizeof(void**);
	}

	allocation_method get_alloc_method() const
	{
		return *reinterpret_cast<allocation_method*>(data);
	}

	void** get_first_block_address_address() const
	{
		return reinterpret_cast<void**>(data + sizeof(allocation_method) + sizeof(Memory*) + sizeof(logger*));
	}

	void* get_first_block_address() const
	{
		return *get_first_block_address_address();
	}

	size_t get_available_block_service_size() const
	{
		return sizeof(size_t) + sizeof(void**);
	};

	size_t get_occupied_block_service_size() const
	{
		return sizeof(size_t);
	};

	size_t get_block_size(void* block_ptr) const
	{
		return *reinterpret_cast<size_t*>(block_ptr);
	}

	void** get_available_next_block_address_address(void* available_block_ptr) const
	{
		return reinterpret_cast<void**>(reinterpret_cast<char*>(available_block_ptr) + sizeof(size_t));
	}

	void* get_available_next_block_address(void* available_block_ptr) const
	{
		return *get_available_next_block_address_address(available_block_ptr);
	}

public:

	DefaultMemory(size_t data_size, allocation_method method, Memory* alloc = nullptr, logger* log = nullptr)
	{
		if (data_size < get_service_block_size() + get_available_block_service_size())
			throw std::bad_alloc();

		if (alloc == nullptr)
		{
			try
			{
				data = reinterpret_cast<char*>(::operator new(data_size));
			}
			catch (...)
			{
				this->log("Alloc error", logger::severity::error);
				throw;
			}
		}
		else
			data = reinterpret_cast<char*>(alloc->allocate(data_size));

		if (data == nullptr)
		{
			this->log("Alloc error", logger::severity::error);
			throw std::bad_alloc();
		}

		char* current = data;

		auto* alloc_method = reinterpret_cast<allocation_method*>(current);
		*alloc_method = method;
		current += sizeof(allocation_method);

		auto** alloc_ptr = reinterpret_cast<Memory**>(current);
		*alloc_ptr = alloc;
		current += sizeof(Memory*);

		auto** log_ = reinterpret_cast<logger**>(current);
		*log_ = log;
		current += sizeof(logger*);

		auto** first_available_block = reinterpret_cast<void**>(current);
		current += sizeof(void*);
		*first_available_block = reinterpret_cast<void*>(current);

		size_t free_size;
		free_size = data_size - get_service_block_size();

		auto* block_size = reinterpret_cast<size_t*>(current);
		*block_size = free_size - get_available_block_service_size();
		current += sizeof(size_t);

		auto** next_ptr = reinterpret_cast<void**>(current);
		*next_ptr = nullptr;

		if (log != nullptr)
		{
			current += sizeof(size_t);
			std::stringstream log_stream;
			log_stream << "Allocated " << *block_size << " bytes [ " << current - data << " ] ";
			this->log(log_stream.str(), logger::severity::information);
		}
	}

	DefaultMemory() : DefaultMemory(ALLOC_SIZE, ALLOC_METHOD) {}

	~DefaultMemory() override
	{
		char* current = data;
		current += sizeof(allocation_method);
		Memory* alloc = *reinterpret_cast<Memory**>(current);
		if (alloc == nullptr)
			::operator delete(data);
		else
			alloc->deallocate(data);
	}

	void* allocate(size_t requested_size) const override
	{
		if (requested_size < sizeof(void*)) // because after deallocate block has (size_t, void*)
			requested_size = sizeof(void*);

		void* previous_block = nullptr;
		void* current_block = get_first_block_address();
		void* target_block = nullptr;
		void* previous_to_target_block = nullptr;
		void* next_to_target_block = nullptr;
		auto alloc_method = get_alloc_method();

		while (current_block != nullptr)
		{
			auto current_block_size = get_block_size(current_block);
			auto* next_block = get_available_next_block_address(current_block);

			if (current_block_size + get_available_block_service_size() >=
				requested_size + get_occupied_block_service_size())
			{
				if (alloc_method == first ||
					alloc_method == best &&
					(target_block == nullptr || current_block_size < get_block_size(target_block)) ||
					alloc_method == worst &&
					(target_block == nullptr || current_block_size > get_block_size(target_block)))
				{
					previous_to_target_block = previous_block;
					target_block = current_block;
					next_to_target_block = next_block;
				}

				if (alloc_method == first)
					break;
			}
			previous_block = current_block;
			current_block = next_block;
		}

		if (target_block == nullptr)
		{
			this->log("Not available memory", logger::severity::error);
			throw std::bad_alloc();
		}

		bool is_requested_size_overridden = false;
		if (get_block_size(target_block) + get_available_block_service_size() - get_occupied_block_service_size() -
			requested_size < get_available_block_service_size())
		{
			// increased to the next block, because can`t split
			requested_size = get_block_size(target_block);
			is_requested_size_overridden = true;
		}

		void* update_next_block_in_previous;

		if (is_requested_size_overridden)
		{
			update_next_block_in_previous = next_to_target_block;
		}
		else
		{
			update_next_block_in_previous = reinterpret_cast<void*>
			(reinterpret_cast<char*>(target_block) + get_occupied_block_service_size() + requested_size);

			auto* target_block_leftover_size = reinterpret_cast<size_t*>(update_next_block_in_previous);
			*target_block_leftover_size =
					get_block_size(target_block) - get_occupied_block_service_size() - requested_size;

			auto* target_block_leftover_next_block_address = reinterpret_cast<void**>(target_block_leftover_size + 1);
			*target_block_leftover_next_block_address = next_to_target_block;
		}

		previous_to_target_block == nullptr
		? *get_first_block_address_address() = update_next_block_in_previous
		: *reinterpret_cast<void**>
		(reinterpret_cast<char*>(previous_to_target_block) + sizeof(size_t)) = update_next_block_in_previous;

		auto* target_block_size_address = reinterpret_cast<size_t*>(target_block);
		*target_block_size_address = requested_size;

		if (this->has_logger())
		{
			char* ptr = reinterpret_cast<char*>(target_block_size_address + 1);
			std::stringstream log_stream;
			if (alloc_method == first)
				log_stream << "Allocated [FIRST] [ " << ptr - data << " ] ";
			else if (alloc_method == best)
				log_stream << "Allocated [BEST] [ " << ptr - data << " ] ";
			else
				log_stream << "Allocated [WORST] [ " << ptr - data << " ] ";
			this->log(log_stream.str(), logger::severity::information);
		}

		return reinterpret_cast<void*>(target_block_size_address + 1);
	}

	void deallocate(void* target_to_dealloc) const override
	{
		*const_cast<void**>(&target_to_dealloc) = reinterpret_cast<void*>
		(reinterpret_cast<size_t*>(target_to_dealloc) - 1);

		if (this->has_logger())
		{
			size_t block_size = *reinterpret_cast<size_t*>(target_to_dealloc);
			std::stringstream log_stream;
			char* block_data = reinterpret_cast<char*>(target_to_dealloc) + sizeof(size_t);
			log_stream << "Deallocated [ " << block_data - data << " ]: ";
			auto* byte_ptr = reinterpret_cast<char*>(block_data);
			for (auto i = 0; i < block_size; i++)
			{
				log_stream << static_cast<unsigned short>(byte_ptr[i]) << " ";
			}
			this->log(log_stream.str(), logger::severity::information);
		}

		void* previous_block = nullptr;
		void* current_block = get_first_block_address();
		if (current_block == nullptr) // target is only one available
		{
			*get_first_block_address_address() = target_to_dealloc;

			auto* block_to_deallocate_size = reinterpret_cast<size_t*>(target_to_dealloc);
			*block_to_deallocate_size -= sizeof(void*);

			*reinterpret_cast<void**>(block_to_deallocate_size + 1) = nullptr;

			return;
		}
		while (current_block != nullptr)
		{
			if (target_to_dealloc < current_block)
			{
				previous_block == nullptr
				? *get_first_block_address_address() = target_to_dealloc
				: *reinterpret_cast<void**>(reinterpret_cast<size_t*>(previous_block) + 1) = target_to_dealloc;

				auto* block_to_deallocate_size = reinterpret_cast<size_t*>(target_to_dealloc);
				*block_to_deallocate_size -= sizeof(void*);

				*reinterpret_cast<void**>(block_to_deallocate_size + 1) = current_block;
				break;
			}
			previous_block = current_block;
			current_block = get_available_next_block_address(current_block);
		}
		if (current_block == nullptr) // target is last free block
		{
			*reinterpret_cast<void**>(reinterpret_cast<size_t*>(previous_block) + 1) = target_to_dealloc;
			auto* block_to_deallocate_size = reinterpret_cast<size_t*>(target_to_dealloc);
			*block_to_deallocate_size -= sizeof(void*);

			*reinterpret_cast<void**>(block_to_deallocate_size + 1) = nullptr;
		}
		else if (reinterpret_cast<char*>(target_to_dealloc) + get_available_block_service_size() +
				 get_block_size(target_to_dealloc) == current_block) // check and merge with right
		{
			void* next_block = get_available_next_block_address(current_block);
			auto* target_size = reinterpret_cast<size_t*>(target_to_dealloc);
			*target_size = get_block_size(target_to_dealloc) + get_block_size(current_block) +
						   get_available_block_service_size();
			auto** next_ptr = reinterpret_cast<void**>(target_size + 1);
			*next_ptr = next_block;
		}
		if (previous_block != nullptr &&
			reinterpret_cast<char*>(previous_block) + get_available_block_service_size() +
			get_block_size(previous_block) == target_to_dealloc) // check and merge with left
		{
			void* next_block = get_available_next_block_address(target_to_dealloc);
			auto* prev_block_size = reinterpret_cast<size_t*>(previous_block);
			*prev_block_size = get_block_size(previous_block) + get_block_size(target_to_dealloc) +
							   get_available_block_service_size();
			auto** prev_next_ptr = reinterpret_cast<void**>(prev_block_size + 1);
			*prev_next_ptr = next_block;
		}
	}
};

#elif CURRENT_ALLOC == BORDER_DESCRIPTOR_ALLOC

class DefaultMemory : public Memory
{
public:

	enum allocation_method
	{
		first, best, worst
	};

private:
	// available block: -
	// occupied block: size_t | prev_occupied_ptr | next_occupied_ptr

	// data: method | alloc_ptr | logger_ptr | first_occupied_ptr | end_memory_ptr (excluding) | ...
	char* data;

	bool has_logger() const
	{
		char* current = data;
		current += (sizeof(allocation_method) + sizeof(Memory*));
		logger* log_ = *reinterpret_cast<logger**>(current);
		return log_ != nullptr;
	};

	void log(std::string const& str, logger::severity severity) const
	{
		char* current = data;
		current += (sizeof(allocation_method) + sizeof(Memory*));
		logger* log_ = *reinterpret_cast<logger**>(current);
		if (log_ == nullptr)
			return;
		log_->log(str, severity);
	}

	size_t get_service_block_size() const
	{
		return sizeof(allocation_method) + sizeof(Memory*) + sizeof(logger*) + 2 * sizeof(void**);
	}

	allocation_method get_alloc_method() const
	{
		return *reinterpret_cast<allocation_method*>(data);
	}

	void** get_first_occupied_block_address_address() const
	{
		return reinterpret_cast<void**>(data + sizeof(allocation_method) + sizeof(Memory*) + sizeof(logger*));
	}

	void* get_memory_end() const
	{
		return *reinterpret_cast<void**>(data + sizeof(allocation_method) + sizeof(Memory*) + sizeof(logger*) +
									   sizeof(void**));
	}

	void* get_first_occupied_block_address() const
	{
		return *get_first_occupied_block_address_address();
	}

	size_t get_occupied_block_service_size() const
	{
		return sizeof(size_t) + 2 * sizeof(void**);
	};

	size_t get_block_size(void* occupied_block) const
	{
		return *reinterpret_cast<size_t*>(occupied_block);
	}

	void** get_next_block_address_address(void* occupied_block) const
	{
		return reinterpret_cast<void**>(reinterpret_cast<char*>(occupied_block) + sizeof(size_t) + sizeof(void**));
	}

	void* get_next_block_address(void* occupied_block) const
	{
		return *get_next_block_address_address(occupied_block);
	}

	void** get_prev_block_address_address(void* occupied_block) const
	{
		return reinterpret_cast<void**>(reinterpret_cast<char*>(occupied_block) + sizeof(size_t));
	}

	void* get_prev_block_address(void* occupied_block) const
	{
		return *get_prev_block_address_address(occupied_block);
	}

public:

	DefaultMemory(size_t data_size, allocation_method method, Memory* alloc = nullptr, logger* log = nullptr)
	{
		if (data_size < get_service_block_size() + get_occupied_block_service_size())
			throw std::bad_alloc();

		if (alloc == nullptr)
		{
			try
			{
				data = reinterpret_cast<char*>(::operator new(data_size));
			}
			catch (...)
			{
				this->log("Alloc error", logger::severity::error);
				throw;
			}
		}
		else
			data = reinterpret_cast<char*>(alloc->allocate(data_size));

		if (data == nullptr)
		{
			this->log("Alloc error", logger::severity::error);
			throw std::bad_alloc();
		}

		char* current = data;

		auto* alloc_method = reinterpret_cast<allocation_method*>(current);
		*alloc_method = method;
		current += sizeof(allocation_method);

		auto** alloc_ptr = reinterpret_cast<Memory**>(current);
		*alloc_ptr = alloc;
		current += sizeof(Memory*);

		auto** log_ = reinterpret_cast<logger**>(current);
		*log_ = log;
		current += sizeof(logger*);

		auto** first_occupied_block = reinterpret_cast<void**>(current);
		*first_occupied_block = nullptr;
		current += sizeof(void**);

		auto** end_memory = reinterpret_cast<void**>(current);
		*end_memory = data + data_size;

		if (log != nullptr)
		{
			current += sizeof(void**);
			std::stringstream log_stream;
			log_stream << "Allocated " << data_size - get_service_block_size() << " bytes [ " << current - data
					   << " ] ";
			this->log(log_stream.str(), logger::severity::information);
		}
	}

	DefaultMemory() : DefaultMemory(ALLOC_SIZE, ALLOC_METHOD) {}

	~DefaultMemory() override
	{
		char* current = data;
		current += sizeof(allocation_method);
		Memory* alloc = *reinterpret_cast<Memory**>(current);
		if (alloc == nullptr)
			::operator delete(data);
		else
			alloc->deallocate(data);
	}

	void* allocate(size_t requested_size) const override
	{
		char* const memory_end = reinterpret_cast<char*>(get_memory_end());
		char* const memory_begin = data + get_service_block_size();
		char* previous_block = nullptr;
		char* current_block = reinterpret_cast<char*>(get_first_occupied_block_address());
		char* target_block = nullptr;
		char* previous_to_target_block = nullptr;
		char* next_to_target_block = nullptr;
		auto alloc_method = get_alloc_method();
		size_t target_block_size;
		if (alloc_method == best)
			target_block_size = SIZE_MAX;
		else if (alloc_method == worst)
			target_block_size = 0;

		bool allocation_ended = false;

		if (current_block == nullptr) // no occupied blocks
		{
			size_t available_block_size = memory_end - memory_begin;
			if (available_block_size >= requested_size + get_occupied_block_service_size())
			{
				target_block = memory_begin;
			}
			allocation_ended = true;
		}
		// check memory between memory begin and first occupied block
		if (!allocation_ended && memory_begin != current_block)
		{
			size_t available_block_size = current_block - memory_begin;
			if (available_block_size >= requested_size + get_occupied_block_service_size())
			{
				target_block_size = available_block_size;
				target_block = memory_begin;
				next_to_target_block = current_block;
			}
		}

		if (!allocation_ended)
		{
			previous_block = current_block;
			current_block = reinterpret_cast<char*>(get_next_block_address(current_block));
		}
		while (current_block != nullptr)
		{
			size_t available_block_size = current_block - (previous_block + get_occupied_block_service_size() +
														   get_block_size(previous_block));

			if (available_block_size >= requested_size + get_occupied_block_service_size())
			{
				if (alloc_method == first ||
					alloc_method == best &&
					(target_block == nullptr || available_block_size < target_block_size) ||
					alloc_method == worst &&
					(target_block == nullptr || available_block_size > target_block_size))
				{
					target_block_size = available_block_size;
					previous_to_target_block = previous_block;
					target_block = previous_block + get_occupied_block_service_size() +
								   get_block_size(previous_block);
					next_to_target_block = current_block;
				}

				if (alloc_method == first)
				{
					allocation_ended = true;
					break;
				}
			}
			previous_block = current_block;
			current_block = reinterpret_cast<char*>(get_next_block_address(current_block));
		}
		// check memory between last occupied block and memory end
		if (!allocation_ended &&
			memory_end != previous_block + get_occupied_block_service_size() + get_block_size(previous_block))
		{
			size_t available_block_size =
					memory_end - (previous_block + get_occupied_block_service_size() + get_block_size(previous_block));
			if (available_block_size >= requested_size + get_occupied_block_service_size())
			{
				if (alloc_method == first ||
					alloc_method == best &&
					(target_block == nullptr || available_block_size < target_block_size) ||
					alloc_method == worst &&
					(target_block == nullptr || available_block_size > target_block_size))
				{
					target_block_size = available_block_size;
					previous_to_target_block = previous_block;
					target_block = previous_block + get_occupied_block_service_size() + get_block_size(previous_block);
					next_to_target_block = nullptr;
				}
			}
		}

		if (target_block == nullptr)
		{
			this->log("Not available memory", logger::severity::error);
			throw std::bad_alloc();
		}

		auto* target_size = reinterpret_cast<size_t*>(target_block);
		*target_size = requested_size;

		auto** target_previous_block = reinterpret_cast<void**>(target_size + 1);
		*target_previous_block = previous_to_target_block;

		auto** target_next_block = target_previous_block + 1;
		*target_next_block = next_to_target_block;

		if (previous_to_target_block == nullptr)
		{
			*get_first_occupied_block_address_address() = target_block;
		}
		else
		{
			auto** previous_block_next_block = reinterpret_cast<void**>(previous_to_target_block + sizeof(size_t) +
																		sizeof(void**));
			*previous_block_next_block = target_block;
		}

		if (next_to_target_block != nullptr)
		{
			auto** next_block_previous_block = reinterpret_cast<void**>(next_to_target_block + sizeof(size_t));
			*next_block_previous_block = target_block;
		}

		if (this->has_logger())
		{
			char* ptr = target_block + get_occupied_block_service_size();
			std::stringstream log_stream;
			if (alloc_method == first)
				log_stream << "Allocated [FIRST] [ " << ptr - data << " ] ";
			else if (alloc_method == best)
				log_stream << "Allocated [BEST] [ " << ptr - data << " ] ";
			else
				log_stream << "Allocated [WORST] [ " << ptr - data << " ] ";
			this->log(log_stream.str(), logger::severity::information);
		}

		return reinterpret_cast<void*>(target_block + get_occupied_block_service_size());
	}

	void deallocate(void* target_to_dealloc) const override
	{
		*const_cast<void**>(&target_to_dealloc) = reinterpret_cast<void*>
		(reinterpret_cast<char*>(target_to_dealloc) - get_occupied_block_service_size());

		if (this->has_logger())
		{
			size_t block_size = *reinterpret_cast<size_t*>(target_to_dealloc);
			std::stringstream log_stream;
			char* block_data = reinterpret_cast<char*>(target_to_dealloc) + sizeof(size_t);
			log_stream << "Deallocated [ " << block_data - data << " ]: ";
			auto* byte_ptr = reinterpret_cast<char*>(block_data);
			for (auto i = 0; i < block_size; i++)
			{
				log_stream << static_cast<unsigned short>(byte_ptr[i]) << " ";
			}
			this->log(log_stream.str(), logger::severity::information);
		}

		auto* previous_block = *reinterpret_cast<void**>(reinterpret_cast<char*>(target_to_dealloc) +
														 sizeof(size_t));
		auto* next_block = *reinterpret_cast<void**>(reinterpret_cast<char*>(target_to_dealloc) +
													 sizeof(size_t) +
													 sizeof(void**));
		if (previous_block == nullptr && next_block == nullptr)
		{
			*get_first_occupied_block_address_address() = nullptr;
		}
		else if (previous_block == nullptr)
		{
			*get_first_occupied_block_address_address() = next_block;
			auto** next_block_previous_block = reinterpret_cast<void**>(reinterpret_cast<size_t*>(next_block) + 1);
			*next_block_previous_block = nullptr;
		}
		else if (next_block == nullptr)
		{
			auto** previous_block_next_block = reinterpret_cast<void**>(reinterpret_cast<char*>(previous_block) +
																		sizeof(size_t) + sizeof(void**));
			*previous_block_next_block = nullptr;
		}
		else
		{
			auto** next_block_previous_block = reinterpret_cast<void**>(reinterpret_cast<size_t*>(next_block) + 1);
			*next_block_previous_block = previous_block;

			auto** previous_block_next_block = reinterpret_cast<void**>(reinterpret_cast<char*>(previous_block) +
																		sizeof(size_t) + sizeof(void**));
			*previous_block_next_block = next_block;
		}
	}
};

#elif CURRENT_ALLOC == BODY_ALLOC

class DefaultMemory : public Memory
{
private:
	// available block: is_occupied | degree | prev_available_ptr | next_available_ptr
	// occupied block: is_occupied | degree

	// data: size_t | alloc_ptr | logger_ptr | first_available_ptr | ...
	char* data;

	size_t get_rounded_of_power_2(size_t number) const
	{
		return 1 << static_cast<size_t>(ceil(log2(number)));
	}

	short get_power(size_t number) const
	{
		return static_cast<short>(log2(number));
	}

	bool has_logger() const
	{
		char* current = data;
		current += (sizeof(size_t) + sizeof(Memory*));
		logger* log_ = *reinterpret_cast<logger**>(current);
		return log_ != nullptr;
	};

	void log(std::string const& str, logger::severity severity) const
	{
		char* current = data;
		current += (sizeof(size_t) + sizeof(Memory*));
		logger* log_ = *reinterpret_cast<logger**>(current);
		if (log_ == nullptr)
			return;
		log_->log(str, severity);
	}

	size_t get_service_block_size() const
	{
		return sizeof(size_t) + sizeof(Memory*) + sizeof(logger*) + sizeof(void**);
	}

	size_t get_memory_size() const
	{
		return *reinterpret_cast<size_t*>(data);
	}

	void** get_first_available_block_address_address() const
	{
		return reinterpret_cast<void**>(data + sizeof(size_t) + sizeof(Memory*) + sizeof(logger*));
	}

	void* get_first_available_block_address() const
	{
		return *get_first_available_block_address_address();
	}

	size_t get_available_block_service_size() const
	{
		return sizeof(bool) + sizeof(short) + 2 * sizeof(void**);
	};

	size_t get_occupied_block_service_size() const
	{
		return sizeof(bool) + sizeof(short);
	};

	short* get_block_power_address(void* block) const
	{
		return reinterpret_cast<short*>(reinterpret_cast<bool*>(block) + 1);
	}

	short get_block_power(void* block) const
	{
		return *get_block_power_address(block);
	}

	bool block_is_available(void* block) const
	{
		return !(*reinterpret_cast<bool*>(block));
	}

	void** get_next_block_address_address(void* block) const
	{
		return reinterpret_cast<void**>(reinterpret_cast<char*>(block) + sizeof(bool) + sizeof(short) +
										sizeof(void**));
	}

	void* get_next_block_address(void* block) const
	{
		return *get_next_block_address_address(block);
	}

	void** get_prev_block_address_address(void* block) const
	{
		return reinterpret_cast<void**>(reinterpret_cast<char*>(block) + sizeof(bool) + sizeof(short));
	}

	void* get_prev_block_address(void* block) const
	{
		return *get_prev_block_address_address(block);
	}

public:

	DefaultMemory(size_t data_size, Memory* alloc = nullptr, logger* log = nullptr)
	{
		if (data_size < get_service_block_size() + get_available_block_service_size())
			throw std::bad_alloc();

		size_t memory_size = get_rounded_of_power_2(data_size - get_service_block_size());
		data_size = memory_size + get_service_block_size();

		if (alloc == nullptr)
		{
			try
			{
				data = reinterpret_cast<char*>(::operator new(data_size));
			}
			catch (...)
			{
				this->log("Alloc error", logger::severity::error);
				throw;
			}
		}
		else
			data = reinterpret_cast<char*>(alloc->allocate(data_size));

		if (data == nullptr)
		{
			this->log("Alloc error", logger::severity::error);
			throw std::bad_alloc();
		}

		char* current = data;

		auto* memory_size_ = reinterpret_cast<size_t*>(current);
		*memory_size_ = memory_size;
		current += sizeof(size_t);

		auto** alloc_ptr = reinterpret_cast<Memory**>(current);
		*alloc_ptr = alloc;
		current += sizeof(Memory*);

		auto** log_ = reinterpret_cast<logger**>(current);
		*log_ = log;
		current += sizeof(logger*);

		auto** first_available_block = reinterpret_cast<void**>(current);
		current += sizeof(void**);
		*first_available_block = current;

		auto* block_is_occupied = reinterpret_cast<bool*>(current);
		*block_is_occupied = false;
		current += sizeof(bool);

		auto* block_power = reinterpret_cast<short*>(current);
		*block_power = get_power(memory_size);
		current += sizeof(short);

		auto** previous_block = reinterpret_cast<void**>(current);
		*previous_block = nullptr;
		current += sizeof(void**);

		auto** next_block = reinterpret_cast<void**>(current);
		*next_block = nullptr;

		if (log != nullptr)
		{
			current += sizeof(void**);
			std::stringstream log_stream;
			log_stream << "Allocated " << data_size - get_service_block_size() << " bytes [ " << current - data
					   << " ] ";
			this->log(log_stream.str(), logger::severity::information);
		}
	}

	DefaultMemory() : DefaultMemory(ALLOC_SIZE) {}

	~DefaultMemory() override
	{
		char* current = data;
		current += sizeof(size_t);
		Memory* alloc = *reinterpret_cast<Memory**>(current);
		if (alloc == nullptr)
			::operator delete(data);
		else
			alloc->deallocate(data);
	}

	void* allocate(size_t requested_size) const override
	{
		// after dealloc block additionally has 2 * sizeof(void**)
		size_t size_to_allocate = requested_size < 2 * sizeof(void**) ? 2 * sizeof(void**) : requested_size;
		size_to_allocate += get_occupied_block_service_size();

		void* current_available_block = get_first_available_block_address();

		while (current_available_block != nullptr)
		{
			short power = get_block_power(current_available_block);

			if ((1 << power) >= size_to_allocate)
			{
				void* prev_available_block = get_prev_block_address(current_available_block);
				void* next_available_block = get_next_block_address(current_available_block);

				while (((1 << power) >> 1) > size_to_allocate)
				{
					power--;

					void* buddie = reinterpret_cast<void*>(reinterpret_cast<char*>(current_available_block) +
														   (1 << power));
					auto* buddie_is_occupied = reinterpret_cast<bool*>(buddie);

					if (next_available_block != nullptr)
						*get_prev_block_address_address(next_available_block) = buddie;

					*buddie_is_occupied = false;
					*get_block_power_address(buddie) = power;
					*get_prev_block_address_address(buddie) = current_available_block;
					*get_next_block_address_address(buddie) = next_available_block;

					*get_block_power_address(current_available_block) = power;
					*get_next_block_address_address(current_available_block) = buddie;

					next_available_block = buddie;
				}

				auto* current_is_occupied = reinterpret_cast<bool*>(current_available_block);
				*current_is_occupied = true;

				if (prev_available_block == nullptr)
				{
					auto** first_available_block = get_first_available_block_address_address();
					*first_available_block = next_available_block;
				}
				else
				{
					*get_next_block_address_address(prev_available_block) = next_available_block;
				}

				if (next_available_block != nullptr)
					*get_prev_block_address_address(next_available_block) = prev_available_block;

				if (this->has_logger())
				{
					char* ptr = reinterpret_cast<char*>(get_block_power_address(current_available_block) + 1);
					std::stringstream log_stream;
					log_stream << "Allocated " << size_to_allocate - get_occupied_block_service_size() << " bytes [ "
							   << ptr - data << " ] ";
					this->log(log_stream.str(), logger::severity::information);
				}

				return get_block_power_address(current_available_block) + 1;
			}

			current_available_block = get_next_block_address(current_available_block);
		}

		this->log("Not available memory", logger::severity::error);
		throw std::bad_alloc();
	}

	void* get_buddie(void* block) const
	{
		if (1 << get_block_power(block) == *reinterpret_cast<size_t*>(data))
			return nullptr;

		void* begin_address = data + get_service_block_size();

		size_t block_size = 1 << get_block_power(block);
		size_t relative_address = reinterpret_cast<char*>(block) - reinterpret_cast<char*>(begin_address);
		size_t result_xor = relative_address ^ block_size;

		return reinterpret_cast<void*>(reinterpret_cast<char*>(begin_address) + result_xor);
	}

	void deallocate(void* target_to_dealloc) const override
	{
		*const_cast<void**>(&target_to_dealloc) = reinterpret_cast<void*>
		(reinterpret_cast<char*>(target_to_dealloc) - get_occupied_block_service_size());

		if (this->has_logger())
		{
			size_t block_size = get_rounded_of_power_2(get_block_power(target_to_dealloc))
								- get_occupied_block_service_size();
			std::stringstream log_stream;
			char* block_data = reinterpret_cast<char*>(target_to_dealloc) + get_occupied_block_service_size();
			log_stream << "Deallocated [ " << block_data - data << " ]: ";
			auto* byte_ptr = reinterpret_cast<char*>(block_data);
			for (auto i = 0; i < block_size; i++)
			{
				log_stream << static_cast<unsigned short>(byte_ptr[i]) << " ";
			}
			this->log(log_stream.str(), logger::severity::information);
		}

		char* current_block = reinterpret_cast<char*>(target_to_dealloc);
		char* prev_block = nullptr;
		char* next_block = reinterpret_cast<char*>(get_first_available_block_address());

		while (next_block != nullptr && current_block > next_block)
		{
			prev_block = next_block;
			next_block = reinterpret_cast<char*>(get_next_block_address(next_block));
		}

		auto* current_is_occupied = reinterpret_cast<bool*>(current_block);
		*current_is_occupied = false;
		*get_prev_block_address_address(current_block) = prev_block;
		*get_next_block_address_address(current_block) = next_block;

		if (prev_block == nullptr)
		{
			auto** first_available_block = get_first_available_block_address_address();
			*first_available_block = current_block;
		}
		else
		{
			*get_next_block_address_address(prev_block) = current_block;
		}

		if (next_block != nullptr)
			*get_prev_block_address_address(next_block) = current_block;

		char* buddie = reinterpret_cast<char*>(get_buddie(current_block));

		while (buddie != nullptr && block_is_available(buddie) &&
			   get_block_power(buddie) == get_block_power(current_block))
		{
			if (buddie < current_block)
			{
				auto* temp = buddie;
				buddie = current_block;
				current_block = temp;
			}

			void* next_block_buddie = get_next_block_address(buddie);
			*get_next_block_address_address(current_block) = next_block_buddie;
			(*get_block_power_address(current_block))++;

			if (next_block_buddie != nullptr)
				*get_prev_block_address_address(next_block_buddie) = current_block;

			buddie = reinterpret_cast<char*>(get_buddie(current_block));
		}
	}
};

#else
#error "Неизвестное значение CURRENT_ALLOC"
#endif
#else
#error "CURRENT_ALLOC не определено"
#endif


#endif //PROGC_SRC_BPLUSTREE_ALLOCATORS_DEFAULT_MEMORY_H
