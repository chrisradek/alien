#ifndef MODEL_DEFINITIONS_H
#define MODEL_DEFINITIONS_H

#include "Base/Definitions.h"
#include "Model/Metadata/CellMetadata.h"
#include "Model/Metadata/CellClusterMetadata.h"
#include "Model/Metadata/EnergyParticleMetadata.h"

#include "DllExport.h"

class Cell;
class Cluster;
class UnitGrid;
class Particle;
class Token;
class CellFeature;
class Unit;
class CellMap;
class EnergyParticleMap;
class SpaceMetricApi;
class SpaceMetric;
class MapCompartment;
class UnitGrid;
class UnitThreadController;
class UnitContext;
class SimulationContext;
class SimulationContextApi;
class SimulationParameters;
class SimulationController;
class SymbolTable;
class UnitObserver;
class EntityFactory;
class ContextFactory;
struct DataChangeDescription;
struct DataLightDescription;
struct ClusterChangeDescription;
struct CellChangeDescription;
struct ParticleChangeDescription;
class SimulationAccess;
class QTimer;
class ModelBuilderFacade;
class SerializationFacade;
class CellConnector;

struct CellClusterHash
{
	std::size_t operator()(Cluster* const& s) const;
};
typedef std::unordered_set<Cluster*, CellClusterHash> CellClusterSet;

struct CellHash
{
    std::size_t operator()(Cell* const& s) const;
};
typedef std::unordered_set<Cell*, CellHash> CellSet;


#endif // MODEL_DEFINITIONS_H
