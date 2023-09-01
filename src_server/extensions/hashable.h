
#ifndef PROGC_SRC_COLLECTIONS_HASHABLE_H
#define PROGC_SRC_COLLECTIONS_HASHABLE_H

#include <cstddef>

class Hashable
{
public:

	virtual size_t hashcode() const = 0;

	virtual ~Hashable() = default;
};

#endif //PROGC_SRC_COLLECTIONS_HASHABLE_H
