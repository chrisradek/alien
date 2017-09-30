#include "CellFeature.h"

CellFeature::~CellFeature ()
{
    delete _nextFeature;
}

void CellFeature::setContext(UnitContext * context)
{
	_context = context;
	if (_nextFeature) {
		_nextFeature->setContext(context);
	}
}

CellFeatureDescription CellFeature::getDescription() const
{
	CellFeatureDescription result;
	CellFeature const* feature = this;
	while (feature) {
		getDescriptionImpl(result);
		feature = feature->_nextFeature;
	}
	return result;
}

void CellFeature::registerNextFeature (CellFeature* nextFeature)
{
    _nextFeature = nextFeature;
}

CellFeature::ProcessingResult CellFeature::process (Token* token, Cell* cell, Cell* previousCell)
{
    ProcessingResult resultFromThisFeature = processImpl(token, cell, previousCell);
    ProcessingResult resultFromNextFeature {false, nullptr };
    if( _nextFeature)
        resultFromNextFeature = _nextFeature->process(token, cell, previousCell);
    ProcessingResult mergedResult;
    mergedResult.decompose = resultFromThisFeature.decompose | resultFromNextFeature.decompose;
    if( resultFromThisFeature.newEnergyParticle )
        mergedResult.newEnergyParticle = resultFromThisFeature.newEnergyParticle;
    else
        mergedResult.newEnergyParticle = resultFromNextFeature.newEnergyParticle;
    return mergedResult;
}

void CellFeature::mutate()
{
	mutateImpl();
	if (_nextFeature) {
		_nextFeature->mutate();
	}
}

