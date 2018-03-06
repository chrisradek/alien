#pragma once

#include "Model/Api/Definitions.h"
#include "Model/Local/UnitContext.h"

class EntityWithTimestamp
{
public:
	inline EntityWithTimestamp(UnitContext* context);
	virtual ~EntityWithTimestamp() = default;

	virtual void setContext(UnitContext * context);
	inline void incTimestampIfFit();

protected:
	inline bool isTimestampFitting() const;

	UnitContext* _context = nullptr;

private:
	uint64_t _timestamp = 0;
};

/********************* inline methods ******************/
EntityWithTimestamp::EntityWithTimestamp(UnitContext* context)
{
	_context = context;
	_timestamp = _context->getTimestamp();
}

void EntityWithTimestamp::incTimestampIfFit()
{
	if (isTimestampFitting()) {
		++_timestamp;
	}
}

bool EntityWithTimestamp::isTimestampFitting() const
{
	return _timestamp == _context->getTimestamp();
}

