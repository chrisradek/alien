#include "DataManipulator.h"

#include "Model/Api/SimulationAccess.h"
#include "Model/Api/SimulationContext.h"
#include "Model/Api/SimulationParameters.h"
#include "Model/Api/DescriptionHelper.h"

void DataManipulator::init(SimulationAccess * access, DescriptionHelper * connector, SimulationContext* context)
{
	SET_CHILD(_access, access);
	SET_CHILD(_descHelper, connector);
	_parameters = context->getSimulationParameters();

	connect(_access, &SimulationAccess::dataReadyToRetrieve, this, &DataManipulator::dataFromSimulationAvailable, Qt::QueuedConnection);
	connect(this, &DataManipulator::notify, this, &DataManipulator::sendDataChangesToSimulation);
}

DataDescription & DataManipulator::getDataRef()
{
	return _data;
}

CellDescription & DataManipulator::getCellDescRef(uint64_t cellId)
{
	ClusterDescription &clusterDesc = getClusterDescRef(cellId);
	int cellIndex = _navi.cellIndicesByCellIds.at(cellId);
	return clusterDesc.cells->at(cellIndex);
}

ClusterDescription & DataManipulator::getClusterDescRef(uint64_t cellId)
{
	int clusterIndex = _navi.clusterIndicesByCellIds.at(cellId);
	return _data.clusters->at(clusterIndex);
}

ParticleDescription& DataManipulator::getParticleDescRef(uint64_t particleId)
{
	int particleIndex = _navi.particleIndicesByParticleIds.at(particleId);
	return _data.particles->at(particleIndex);
}

void DataManipulator::addAndSelectCell(QVector2D const & posDelta)
{
	QVector2D pos = _rect.center().toQVector2D() + posDelta;
	auto desc = ClusterDescription().setPos(pos).setVel({}).setAngle(0).setAngularVel(0).setMetadata(ClusterMetadata()).addCell(
		CellDescription().setEnergy(_parameters->cellCreationEnergy).setMaxConnections(_parameters->cellCreationMaxConnection)
		.setPos(pos).setConnectingCells({}).setMetadata(CellMetadata())
		.setFlagTokenBlocked(false).setTokenBranchNumber(0).setCellFeature(
			CellFeatureDescription().setType(Enums::CellFunction::COMPUTER)
		));
	_descHelper->makeValid(desc);
	_data.addCluster(desc);
	_selectedCellIds = { desc.cells->front().id };
	_selectedClusterIds = { desc.id };
	_selectedParticleIds = { };
	_navi.update(_data);
}

void DataManipulator::addAndSelectParticle(QVector2D const & posDelta)
{
	QVector2D pos = _rect.center().toQVector2D() + posDelta;
	auto desc = ParticleDescription().setPos(pos).setVel({}).setEnergy(_parameters->cellMinEnergy / 2.0);
	_descHelper->makeValid(desc);
	_data.addParticle(desc);
	_selectedCellIds = { };
	_selectedClusterIds = { };
	_selectedParticleIds = { desc.id };
	_navi.update(_data);
}

bool DataManipulator::isCellPresent(uint64_t cellId)
{
	return _navi.cellIds.find(cellId) != _navi.cellIds.end();
}

bool DataManipulator::isParticlePresent(uint64_t particleId)
{
	return _navi.particleIds.find(particleId) != _navi.particleIds.end();
}

void DataManipulator::dataFromSimulationAvailable()
{
	updateInternals(_access->retrieveData());

	Q_EMIT notify({ Receiver::DataEditor, Receiver::VisualEditor });
}

void DataManipulator::sendDataChangesToSimulation(set<Receiver> const& targets)
{
	if (targets.find(Receiver::Simulation) == targets.end()) {
		return;
	}
	DataChangeDescription delta(_unchangedData, _data);
	_access->updateData(delta);
	_unchangedData = _data;
}

void DataManipulator::setSelection(list<uint64_t> const &cellIds, list<uint64_t> const &particleIds)
{
	_selectedCellIds = set<uint64_t>(cellIds.begin(), cellIds.end());
	_selectedParticleIds = set<uint64_t>(particleIds.begin(), particleIds.end());
	_selectedClusterIds.clear();
	for (uint64_t cellId : cellIds) {
		auto clusterIdByCellIdIter = _navi.clusterIdsByCellIds.find(cellId);
		if (clusterIdByCellIdIter != _navi.clusterIdsByCellIds.end()) {
			_selectedClusterIds.insert(clusterIdByCellIdIter->second);
		}
	}
}

bool DataManipulator::isInSelection(list<uint64_t> const & ids) const
{
	for (uint64_t id : ids) {
		if (!isInSelection(id)) {
			return false;
		}
	}
	return true;
}

bool DataManipulator::isInSelection(uint64_t id) const
{
	return (_selectedCellIds.find(id) != _selectedCellIds.end() || _selectedParticleIds.find(id) != _selectedParticleIds.end());
}

bool DataManipulator::isInExtendedSelection(uint64_t id) const
{
	auto clusterIdByCellIdIter = _navi.clusterIdsByCellIds.find(id);
	if (clusterIdByCellIdIter != _navi.clusterIdsByCellIds.end()) {
		uint64_t clusterId = clusterIdByCellIdIter->second;
		return (_selectedClusterIds.find(clusterId) != _selectedClusterIds.end() || _selectedParticleIds.find(id) != _selectedParticleIds.end());
	}
	return false;
}

bool DataManipulator::areEntitiesSelected() const
{
	return !_selectedCellIds.empty() || !_selectedParticleIds.empty();
}

set<uint64_t> DataManipulator::getSelectedCellIds() const
{
	return _selectedCellIds;
}

set<uint64_t> DataManipulator::getSelectedParticleIds() const
{
	return _selectedParticleIds;
}

void DataManipulator::moveSelection(QVector2D const &delta)
{
	for (uint64_t cellId : _selectedCellIds) {
		if (isCellPresent(cellId)) {
			int clusterIndex = _navi.clusterIndicesByCellIds.at(cellId);
			int cellIndex = _navi.cellIndicesByCellIds.at(cellId);
			CellDescription &cellDesc = getCellDescRef(cellId);
			cellDesc.pos = *cellDesc.pos + delta;
		}
	}

	for (uint64_t particleId : _selectedParticleIds) {
		if (isParticlePresent(particleId)) {
			ParticleDescription &particleDesc = getParticleDescRef(particleId);
			particleDesc.pos = *particleDesc.pos + delta;
		}
	}
}

void DataManipulator::moveExtendedSelection(QVector2D const & delta)
{
	for (uint64_t selectedClusterId : _selectedClusterIds) {
		auto selectedClusterIndex = _navi.clusterIndicesByClusterIds.at(selectedClusterId);
		ClusterDescription &clusterDesc = _data.clusters->at(selectedClusterIndex);
		clusterDesc.pos = *clusterDesc.pos + delta;
	}

	list<uint64_t> extSelectedCellIds;
	for (auto clusterIdByCellId : _navi.clusterIdsByCellIds) {
		uint64_t cellId = clusterIdByCellId.first;
		uint64_t clusterId = clusterIdByCellId.second;
		if (_selectedClusterIds.find(clusterId) != _selectedClusterIds.end()) {
			extSelectedCellIds.push_back(cellId);
		}
	}

	for (uint64_t cellId : extSelectedCellIds) {
		if (isCellPresent(cellId)) {
			int clusterIndex = _navi.clusterIndicesByCellIds.at(cellId);
			int cellIndex = _navi.cellIndicesByCellIds.at(cellId);
			CellDescription &cellDesc = getCellDescRef(cellId);
			cellDesc.pos = *cellDesc.pos + delta;
		}
	}

	for (uint64_t particleId : _selectedParticleIds) {
		if (isParticlePresent(particleId)) {
			ParticleDescription &particleDesc = getParticleDescRef(particleId);
			particleDesc.pos = *particleDesc.pos + delta;
		}
	}
}

void DataManipulator::reconnectSelectedCells()
{
	_descHelper->reconnect(getDataRef(), getSelectedCellIds());
	updateAfterCellReconnections();
}

void DataManipulator::updateCluster(ClusterDescription const & cluster)
{
	int clusterIndex = _navi.clusterIndicesByClusterIds.at(cluster.id);
	_data.clusters->at(clusterIndex) = cluster;

	_navi.update(_data);
}

void DataManipulator::updateParticle(ParticleDescription const & particle)
{
	int particleIndex = _navi.particleIndicesByParticleIds.at(particle.id);
	_data.particles->at(particleIndex) = particle;

	_navi.update(_data);
}

void DataManipulator::requireDataUpdateFromSimulation(IntRect const& rect)
{
	_rect = rect;
	ResolveDescription resolveDesc;
	resolveDesc.resolveCellLinks = true;
	_access->requireData(rect, resolveDesc);
}

void DataManipulator::updateAfterCellReconnections()
{
	_navi.update(_data);

	_selectedClusterIds.clear();
	for (uint64_t selectedCellId : _selectedCellIds) {
		if (_navi.clusterIdsByCellIds.find(selectedCellId) != _navi.clusterIdsByCellIds.end()) {
			_selectedClusterIds.insert(_navi.clusterIdsByCellIds.at(selectedCellId));
		}
	}
}

void DataManipulator::updateInternals(DataDescription const &data)
{
	_data = data;
	_unchangedData = _data;
	_selectedCellIds.clear();
	_selectedClusterIds.clear();
	_selectedParticleIds.clear();
	_navi.update(data);
}

