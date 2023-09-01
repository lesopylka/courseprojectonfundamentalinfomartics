
#ifndef PROGC_SRC_EXTENSIONS_SERIALIZABLE_H
#define PROGC_SRC_EXTENSIONS_SERIALIZABLE_H


#include <string>

class Serializable
{
public:

	virtual std::string serialize() const = 0;

	virtual ~Serializable() = default;
};


#endif //PROGC_SRC_EXTENSIONS_SERIALIZABLE_H
