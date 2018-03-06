#ifndef GLOBALFACTORY_H
#define GLOBALFACTORY_H

#include "Definitions.h"

class GlobalFactory
{
public:
	virtual ~GlobalFactory() = default;

	virtual NumberGenerator* buildRandomNumberGenerator() const = 0;
};

#endif // GLOBALFACTORY_H
