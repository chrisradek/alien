#pragma once

#include <QMutex>

#include "UnitContext.h"

class UnitContextImpl
	: public UnitContext
{
public:
	UnitContextImpl(QObject* parent = nullptr);
	virtual ~UnitContextImpl();

	void init(NumberGenerator* numberGen, SpaceProperties* spaceProp, CellMap* cellMap, ParticleMap* energyMap
		, MapCompartment* mapCompartment, SimulationParameters const& parameters) override;

	virtual NumberGenerator* getNumberGenerator() const override;
	virtual SpaceProperties* getSpaceProperties () const override;
	virtual CellMap* getCellMap () const override;
	virtual ParticleMap* getParticleMap () const override;
	virtual MapCompartment* getMapCompartment() const override;
	virtual SimulationParameters const& getSimulationParameters() const override;

	virtual uint64_t getTimestamp() const override;
	virtual void incTimestamp() override;

	virtual void setSimulationParameters(SimulationParameters const& parameters) override;
	virtual QList<Cluster*>& getClustersRef() override;
	virtual QList<Particle*>& getParticlesRef () override;

private:
	void deleteClustersAndEnergyParticles();

	QList<Cluster*> _clusters;
    QList<Particle*> _energyParticles;
	NumberGenerator* _numberGen = nullptr;
	SpaceProperties* _spaceProperties = nullptr;
    CellMap* _cellMap = nullptr;
    ParticleMap* _energyParticleMap = nullptr;
	MapCompartment* _mapCompartment = nullptr;
	SymbolTable* _symbolTable = nullptr;
	SimulationParameters _simulationParameters;

	uint64_t _timestamp = 0;
};

