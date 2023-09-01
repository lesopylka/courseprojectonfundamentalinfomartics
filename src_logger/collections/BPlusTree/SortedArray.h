#ifndef PROGC_SRC_BPLUSTREE_SORTEDARRAY_H
#define PROGC_SRC_BPLUSTREE_SORTEDARRAY_H


#include <stdexcept>
#include "../allocators/memory.h"


template<typename T>
class SortedArray
{
private:

	T* data;
	const int capacity = 0;
	int size = 0;
	const std::function<int(const T&, const T&)> compare;
	const std::shared_ptr<Memory> alloc;

	SortedArray(int capacity, const std::function<int(const T&, const T&)>& comparator,
		const std::shared_ptr<Memory>& alloc) : compare(comparator), alloc(alloc), capacity(capacity)
	{
	}
	~SortedArray() = default;

public:

	SortedArray(const SortedArray&) = delete;
	SortedArray& operator=(const SortedArray&) = delete;
	SortedArray(SortedArray&&) = delete;
	SortedArray& operator=(SortedArray&&) = delete;

	// return: elem was found, index: found or to insert
	bool binarySearch(int& index, const T& elem)
	{
		if (size == 0)
		{
			index = 0;
			return false;
		}
		int low = 0;
		int high = size - 1;
		bool isFound = false;
		int mid;

		while (low <= high)
		{
			mid = (low + high) >> 1;
			T& midVal = data[mid];

			int cmp = compare(midVal, elem);
			if (cmp < 0)
			{
				low = mid + 1;
			}
			else if (cmp > 0)
			{
				high = mid - 1;
			}
			else
			{
				isFound = true;
				break;
			}
		}
		if (isFound)
		{
			index = mid;
			return true;
		}
		index = low;
		return false;
	}

	static inline SortedArray<T>* create(int capacity, const std::shared_ptr<Memory>& alloc,
		std::function<int(const T&, const T&)> comparator)
	{
		if (capacity < 1)
			throw std::runtime_error("Capacity must be > 0");
		auto* array = reinterpret_cast<SortedArray<T>*>(alloc->allocate(sizeof(SortedArray<T>)));
		new(array) SortedArray<T>(capacity, comparator, alloc);
		array->data = reinterpret_cast<T*>(alloc->allocate(sizeof(T) * capacity));
		return array;
	}

	void destroy()
	{
		alloc->deallocate(data);
		alloc->deallocate(this);
	}

	void clear()
	{
		size = 0;
	}

	T get(int index) const
	{
		if (index < 0 || index >= size)
			throw std::runtime_error("SortedArray:get: Incorrect index");
		return data[index];
	}

	// return: -1 if elem exists or index of elem
	int add(const T& elem)
	{
		if (size == capacity)
			throw std::runtime_error("Array is full");
		int insertIndex;
		if (binarySearch(insertIndex, elem))
		{
			return -1; // elem already exists
		}
		if (insertIndex != size)
			memmove(data + insertIndex + 1, data + insertIndex, sizeof(T) * (size - insertIndex));
		data[insertIndex] = elem;
		size++;
		return insertIndex;
	}

	void remove(int index)
	{
		if (index < 0 || index >= size)
			throw std::runtime_error("SortedArray:remove: Incorrect index");
		if (index != size - 1)
		{
			memmove(data + index, data + index + 1, sizeof(T) * (size - index - 1));
		}
		size--;
	}

	int contains(T& elem)
	{
		int index;
		if (binarySearch(index, elem))
		{
			return index;
		}
		return -1;
	}

	bool removeElem(T& elem)
	{
		int index = contains(elem);
		if (index < 0)
			return false;
		remove(index);
		return true;
	}

	void forEach(const std::function<void(const T&)>& func) const
	{
		for (int x = 0; x < size; x++)
			func(data[x]);
	}

	int getCapacity() const
	{
		return capacity;
	}

	int getSize() const
	{
		return size;
	}

	bool isFull() const
	{
		return size == capacity;
	}

	bool isEmpty() const
	{
		return size == 0;
	}
};


#endif //PROGC_SRC_BPLUSTREE_SORTEDARRAY_H
