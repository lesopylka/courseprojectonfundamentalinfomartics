#ifndef MEMORY_H
#define MEMORY_H

#include <cstddef>

class Memory
{
public:
	virtual void* allocate(size_t target_size) const = 0;

	virtual void deallocate(void* const target_to_dealloc) const = 0;

	virtual ~Memory();
};

#endif // MEMORY_H