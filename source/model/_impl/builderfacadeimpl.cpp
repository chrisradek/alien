#include "global/GlobalFactory.h"
#include "global/NumberGenerator.h"
#include "global/ServiceLocator.h"
#include "model/entities/Cell.h"
#include "model/entities/CellCluster.h"
#include "model/entities/EnergyParticle.h"
#include "model/entities/Token.h"
#include "model/entities/EntityFactory.h"
#include "model/features/CellFunction.h"
#include "model/features/CellFunctionComputer.h"
#include "model/features/EnergyGuidance.h"
#include "model/features/CellFeatureFactory.h"
#include "model/context/SimulationParameters.h"
#include "model/context/CellMap.h"
#include "model/context/EnergyParticleMap.h"
#include "model/context/SpaceMetric.h"
#include "model/context/ContextFactory.h"
#include "model/context/MapCompartment.h"
#include "model/context/UnitThreadController.h"
#include "model/context/UnitGrid.h"
#include "model/context/SimulationContext.h"
#include "model/context/Unit.h"
#include "model/context/UnitContext.h"
#include "model/AccessPorts/AccessPortsFactory.h"
#include "model/AccessPorts/SimulationAccess.h"
#include "model/metadata/SymbolTable.h"
#include "model/ModelSettings.h"
#include "model/_impl/SimulationControllerImpl.h"

#include "BuilderFacadeImpl.h"

namespace {
	BuilderFacadeImpl instance;
}

BuilderFacadeImpl::BuilderFacadeImpl ()
{
    ServiceLocator::getInstance().registerService<BuilderFacade>(this);
}

SimulationFullAccess * BuilderFacadeImpl::buildSimulationFullAccess(SimulationContextApi * context) const
{
	AccessPortsFactory* factory = ServiceLocator::getInstance().getService<AccessPortsFactory>();
	auto access = factory->buildSimulationFullAccess();
	access->init(context);
	return access;
}

SimulationLightAccess * BuilderFacadeImpl::buildSimulationLightAccess(SimulationContextApi * context) const
{
	AccessPortsFactory* factory = ServiceLocator::getInstance().getService<AccessPortsFactory>();
	auto access = factory->buildSimulationLightAccess();
	access->init(context);
	return access;
}

SimulationController * BuilderFacadeImpl::buildSimulationController(SimulationContextApi * context) const
{
	auto controller = new SimulationControllerImpl();
	controller->init(static_cast<SimulationContext*>(context));
	return controller;
}

SimulationContextApi* BuilderFacadeImpl::buildSimulationContext(int maxRunngingThreads, IntVector2D gridSize, SpaceMetric* metric, SymbolTable* symbolTable
	, SimulationParameters* parameters) const
{
	ContextFactory* contextFactory = ServiceLocator::getInstance().getService<ContextFactory>();
	GlobalFactory* globalFactory = ServiceLocator::getInstance().getService<GlobalFactory>();
	SimulationContext* context = contextFactory->buildSimulationContext();

	auto threads = contextFactory->buildSimulationThreads();
	auto grid = contextFactory->buildSimulationGrid();
	auto numberGen = globalFactory->buildRandomNumberGenerator();
	threads->init(maxRunngingThreads);
	grid->init(gridSize, metric);
	context->init(numberGen, metric, grid, threads, symbolTable, parameters);

	for (int x = 0; x < gridSize.x; ++x) {
		for (int y = 0; y < gridSize.y; ++y) {
			auto unit = buildSimulationUnit({ x,y }, context);
			grid->registerUnit({ x,y }, unit);
			threads->registerUnit(unit);
		}
	}

	for (int x = 0; x < gridSize.x; ++x) {
		for (int y = 0; y < gridSize.y; ++y) {
			auto unitContext = grid->getUnitOfGridPos({ x, y })->getContext();
			auto compartment = unitContext->getMapCompartment();
			auto getContextFromDelta = [&](IntVector2D const& delta) {
				return grid->getUnitOfGridPos({ (x + delta.x + gridSize.x) % gridSize.x, (y + delta.y + gridSize.y) % gridSize.y })->getContext();
			};
			compartment->registerNeighborContext(MapCompartment::RelativeLocation::UpperLeft, getContextFromDelta({ -1, -1 }));
			compartment->registerNeighborContext(MapCompartment::RelativeLocation::Upper, getContextFromDelta({ 0, -1 }));
			compartment->registerNeighborContext(MapCompartment::RelativeLocation::UpperRight, getContextFromDelta({ +1, -1 }));
			compartment->registerNeighborContext(MapCompartment::RelativeLocation::Left, getContextFromDelta({ -1, 0 }));
			compartment->registerNeighborContext(MapCompartment::RelativeLocation::Right, getContextFromDelta({ +1, 0 }));
			compartment->registerNeighborContext(MapCompartment::RelativeLocation::LowerLeft, getContextFromDelta({ -1, +1 }));
			compartment->registerNeighborContext(MapCompartment::RelativeLocation::Lower, getContextFromDelta({ 0, +1 }));
			compartment->registerNeighborContext(MapCompartment::RelativeLocation::LowerRight, getContextFromDelta({ +1, +1 }));
		}
	}

	return context;
}

namespace
{
	const int ARRAY_SIZE_FOR_RANDOM_NUMBERS = 234327;
}

Unit * BuilderFacadeImpl::buildSimulationUnit(IntVector2D gridPos, SimulationContext* context) const
{
	ContextFactory* contextFactory = ServiceLocator::getInstance().getService<ContextFactory>();
	GlobalFactory* globalFactory = ServiceLocator::getInstance().getService<GlobalFactory>();
	auto grid = context->getUnitGrid();
	auto threads = context->getUnitThreadController();

	auto unit = contextFactory->buildSimulationUnit();		//unit has no parent due to an QObject::moveToThread call later
	auto unitContext = contextFactory->buildSimulationUnitContext();
	auto numberGen = globalFactory->buildRandomNumberGenerator();
	auto metric = context->getSpaceMetric()->clone();
	auto compartment = contextFactory->buildMapCompartment();
	auto cellMap = contextFactory->buildCellMap();
	auto energyMap = contextFactory->buildEnergyParticleMap();
	auto symbolTable = context->getSymbolTable()->clone();
	auto parameters = context->getSimulationParameters()->clone();
	uint16_t threadId = gridPos.x + gridPos.y * grid->getSize().x;
	numberGen->init(ARRAY_SIZE_FOR_RANDOM_NUMBERS, threadId);
	compartment->init(grid->calcCompartmentRect(gridPos));
	cellMap->init(metric, compartment);
	energyMap->init(metric, compartment);
	unitContext->init(numberGen, metric, cellMap, energyMap, compartment, symbolTable, parameters);
	unit->init(unitContext);

	return unit;
}

SpaceMetric * BuilderFacadeImpl::buildSpaceMetric(IntVector2D universeSize) const
{
	ContextFactory* factory = ServiceLocator::getInstance().getService<ContextFactory>();
	auto metric = factory->buildSpaceMetric();
	metric->init(universeSize);
	return metric;
}

SymbolTable * BuilderFacadeImpl::buildDefaultSymbolTable() const
{
	return ModelSettings::loadDefaultSymbolTable();
}

SimulationParameters * BuilderFacadeImpl::buildDefaultSimulationParameters() const
{
	return ModelSettings::loadDefaultSimulationParameters();
}

CellCluster* BuilderFacadeImpl::buildCellCluster (UnitContext* context) const
{
    EntityFactory* entityFactory = ServiceLocator::getInstance().getService<EntityFactory>();
    return entityFactory->buildCellCluster(context);
}

CellCluster* BuilderFacadeImpl::buildCellCluster (QList< Cell* > cells, qreal angle, QVector3D pos, qreal angularVel
    , QVector3D vel, UnitContext* context) const
{
    EntityFactory* entityFactory = ServiceLocator::getInstance().getService<EntityFactory>();
    return entityFactory->buildCellCluster(cells, angle, pos, angularVel, vel, context);
}


Cell* BuilderFacadeImpl::buildFeaturedCell (qreal energy, Enums::CellFunction::Type type, QByteArray data
    , UnitContext* context, int maxConnections, int tokenAccessNumber, QVector3D relPos) const
{
    EntityFactory* entityFactory = ServiceLocator::getInstance().getService<EntityFactory>();
    CellFeatureFactory* decoratorFactory = ServiceLocator::getInstance().getService<CellFeatureFactory>();
    Cell* cell = entityFactory->buildCell(energy, context, maxConnections, tokenAccessNumber, relPos);
    decoratorFactory->addCellFunction(cell, type, data, context);
    decoratorFactory->addEnergyGuidance(cell, context);
    return cell;
}

Cell* BuilderFacadeImpl::buildFeaturedCell (qreal energy, Enums::CellFunction::Type type, UnitContext* context
    , int maxConnections, int tokenAccessNumber, QVector3D relPos) const
{
    EntityFactory* entityFactory = ServiceLocator::getInstance().getService<EntityFactory>();
    CellFeatureFactory* decoratorFactory = ServiceLocator::getInstance().getService<CellFeatureFactory>();
    Cell* cell = entityFactory->buildCell(energy, context, maxConnections, tokenAccessNumber, relPos);
    decoratorFactory->addCellFunction(cell, type, context);
    decoratorFactory->addEnergyGuidance(cell, context);
    return cell;
}

Cell* BuilderFacadeImpl::buildFeaturedCellWithRandomData (qreal energy, UnitContext* context) const
{
	SimulationParameters* parameters = context->getSimulationParameters();
    int randomMaxConnections = context->getNumberGenerator()->getRandomInt(parameters->cellMaxBonds+1);
    int randomTokenAccessNumber = context->getNumberGenerator()->getRandomInt(parameters->cellMaxTokenBranchNumber);
    QByteArray randomData(256, 0);
	for (int i = 0; i < 256; ++i) {
		randomData[i] = context->getNumberGenerator()->getRandomInt(256);
	}
    Enums::CellFunction::Type randomCellFunction = static_cast<Enums::CellFunction::Type>(context->getNumberGenerator()->getRandomInt(Enums::CellFunction::_COUNTER));
    return buildFeaturedCell(energy, randomCellFunction, randomData, context, randomMaxConnections, randomTokenAccessNumber, QVector3D());
}

Token* BuilderFacadeImpl::buildToken(UnitContext* context, qreal energy) const
{
	EntityFactory* entityFactory = ServiceLocator::getInstance().getService<EntityFactory>();
	return entityFactory->buildToken(context, energy);
}

CellTO BuilderFacadeImpl::buildFeaturedCellTO (Cell* cell) const
{
    CellTO to;

    //copy cell properties
    CellCluster* cluster = cell->getCluster();
    to.numCells = cluster->getMass();
    to.clusterPos = cluster->getPosition();
    to.clusterVel = cluster->getVelocity();
    to.clusterAngle = cluster->getAngle();
    to.clusterAngVel = cluster->getAngularVel();
    to.cellPos = cell->calcPosition();
    to.cellEnergy = cell->getEnergy();
    to.cellNumCon = cell->getNumConnections();
    to.cellMaxCon = cell->getMaxConnections();
    to.cellAllowToken = !cell->isTokenBlocked();
    to.cellTokenAccessNum = cell->getBranchNumber();
    CellFunction* cellFunction = cell->getFeatures()->findObject<CellFunction>();
    to.cellFunctionType = cellFunction->getType();

    //copy computer data
    CellFunctionComputer* computer = cellFunction->findObject<CellFunctionComputer>();
    if( computer ) {
        QByteArray d = computer->getMemoryReference();
		to.computerMemory = d;
        to.computerCode = computer->decompileInstructionCode();
    }

    //copy token data
    for(int i = 0; i < cell->getNumToken(); ++i) {
        Token* token = cell->getToken(i);
        to.tokenEnergies << token->getEnergy();
        to.tokenData << token->getMemoryRef();
    }
    return to;
}
