#pragma once

#include <cuda_runtime.h>

#include "CudaShared.cuh"
#include "CudaBase.cuh"
#include "CudaConstants.cuh"
#include "CudaMap.cuh"
#include "CudaSimulationData.cuh"

struct CollisionEntry
{
	ClusterCuda* cluster;
	int numCollisions;
	double2 collisionPos;
	double2 normalVec;
};

class CollisionData
{
public:
	
	int numEntries;
	CollisionEntry entries[MAX_COLLIDING_CLUSTERS];

	__device__ void init_Kernel()
	{
		numEntries = 0;
	}

	__device__ CollisionEntry* getOrCreateEntry(ClusterCuda* cluster)
	{
		int old;
		int curr;
		do {
			for (int i = 0; i < numEntries; ++i) {
				if (entries[i].cluster = cluster) {
					return &entries[i];
				}
			}
			
			old = numEntries;
			curr = atomicCAS(&numEntries, old, (old + 1) % MAX_COLLIDING_CLUSTERS);
			if (old == curr) {
				auto result = &entries[old];
				result->cluster = cluster;
				result->numCollisions = 0;
				result->collisionPos = { 0, 0 };
				result->normalVec = { 0, 0 };
				return result;
			}

		} while (true);
	}
};

__device__ __inline__ void rotateQuarterCounterClockwise_Kernel(double2 &v)
{
	double temp = v.x;
	v.x = v.y;
	v.y = -temp;
}

__device__ __inline__ double2 calcNormalToCell_Kernel(CellCuda *cell, double2 outward)
{
	normalize(outward);
	if (cell->numConnections < 2) {
		return outward;
	}

	CellCuda* minCell = nullptr;
	CellCuda* maxCell = nullptr;
	double2 minVector;
	double2 maxVector;
	double min_h = 0.0;	//h = angular distance from outward vector
	double max_h = 0.0;

	for (int i = 0; i < cell->numConnections; ++i) {

		//calculate h (angular distance from outward vector)
		double2 u = sub(cell->connections[i]->absPos, cell->absPos);
		normalize(u);
		double h = dot(outward, u);
		if (outward.x*u.y - outward.y*u.x < 0.0) {
			h = -2 - h;
		}

		if (!minCell || h < min_h) {
			minCell = cell->connections[i];
			minVector = u;
			min_h = h;
		}
		if (!maxCell || h > max_h) {
			maxCell = cell->connections[i];
			maxVector = u;
			max_h = h;
		}
	}

	//no adjacent cells?
	if (!minCell && !maxCell) {
		return outward;
	}

	//one adjacent cells?
	if (minCell == maxCell) {
		return sub(cell->absPos, minCell->absPos);
	}

	//calc normal vectors
	double temp = maxVector.x;
	maxVector.x = maxVector.y;
	maxVector.y = -temp;
	temp = minVector.x;
	minVector.x = -minVector.y;
	minVector.y = temp;
	auto result = add(minVector, maxVector);
	normalize(result);
	return result;
}
__device__ __inline__ void calcCollision_Kernel(ClusterCuda *clusterA, CollisionEntry *collisionEntry, int2 const& size)
{
	double2 posA = clusterA->pos;
	double2 velA = clusterA->vel;
	double angVelA = clusterA->angularVel * DEG_TO_RAD;
	double angMassA = clusterA->angularMass;

	ClusterCuda *clusterB = collisionEntry->cluster;
	double2 posB = clusterB->pos;
	double2 velB = clusterB->vel;
	double angVelB = clusterB->angularVel * DEG_TO_RAD;
	double angMassB = clusterB->angularMass;

	double2 rAPp = sub(collisionEntry->collisionPos, posA);
	mapDisplacementCorrection_Kernel(rAPp, size);
	rotateQuarterCounterClockwise_Kernel(rAPp);
	double2 rBPp = sub(collisionEntry->collisionPos, posB);
	mapDisplacementCorrection_Kernel(rBPp, size);
	rotateQuarterCounterClockwise_Kernel(rBPp);
	double2 vAB = sub(sub(velA, mul(rAPp, angVelA)), sub(velB, mul(rBPp, angVelB)));

	double2 n = collisionEntry->normalVec;

	if (angMassA < FP_PRECISION) {
		angVelA = 0.0;
	}
	if (angMassB < FP_PRECISION) {
		angVelB = 0.0;
	}

	double vAB_dot_n = dot(vAB, n);
	if (vAB_dot_n > 0.0) {
		return;
	}

	double massA = static_cast<double>(clusterA->numCells);
	double massB = static_cast<double>(clusterB->numCells);
	double rAPp_dot_n = dot(rAPp, n);
	double rBPp_dot_n = dot(rBPp, n);

	if (angMassA > FP_PRECISION && angMassB > FP_PRECISION) {
		double j = -2.0*vAB_dot_n / ((1.0/massA + 1.0/massB)
			+ rAPp_dot_n * rAPp_dot_n / angMassA + rBPp_dot_n * rBPp_dot_n / angMassB);


		atomicAdd(&clusterA->angularVel, -(rAPp_dot_n * j / angMassA) * RAD_TO_DEG);
		atomicAdd(&clusterA->vel.x, j / massA * n.x);
		atomicAdd(&clusterA->vel.y, j / massA * n.y);

/*
			atomicAdd(&collisionData.angularVelDelta, (rBPp_dot_n * j / angMassB) * RAD_TO_DEG);
			atomicAdd(&collisionData.velDelta.x, -j / massB * n.x);
			atomicAdd(&collisionData.velDelta.y, -j / massB * n.y);
*/
	}

	if (angMassA <= FP_PRECISION && angMassB > FP_PRECISION) {
		double j = -2.0*vAB_dot_n / ((1.0 / massA + 1.0 / massB)
			 + rBPp_dot_n * rBPp_dot_n / angMassB);

			atomicAdd(&clusterA->vel.x, j / massA * n.x);
			atomicAdd(&clusterA->vel.y, j / massA * n.y);
/*
			atomicAdd(&collisionData.angularVelDelta, (rBPp_dot_n * j / angMassB) * RAD_TO_DEG);
			atomicAdd(&collisionData.velDelta.x, -j / massB * n.x);
			atomicAdd(&collisionData.velDelta.y, -j / massB * n.y);
*/
	}

	if (angMassA > FP_PRECISION && angMassB <= FP_PRECISION) {
		double j = -2.0*vAB_dot_n / ((1.0 / massA + 1.0 / massB)
			+ rAPp_dot_n * rAPp_dot_n / angMassA);

		atomicAdd(&clusterA->angularVel, -(rAPp_dot_n * j / angMassA) * RAD_TO_DEG);
		atomicAdd(&clusterA->vel.x, j / massA * n.x);
		atomicAdd(&clusterA->vel.y, j / massA * n.y);
/*
			atomicAdd(&collisionData.velDelta.x, -j / massB * n.x);
			atomicAdd(&collisionData.velDelta.y, -j / massB * n.y);
*/
	}

	if (angMassA <= FP_PRECISION && angMassB <= FP_PRECISION) {
		double j = -2.0*vAB_dot_n / ((1.0 / massA + 1.0 / massB));

		atomicAdd(&clusterA->vel.x, j / massA * n.x);
		atomicAdd(&clusterA->vel.y, j / massA * n.y);
/*
			atomicAdd(&collisionData.velDelta.x, -j / massB * n.x);
			atomicAdd(&collisionData.velDelta.y, -j / massB * n.y);
*/
	}

}

__device__ __inline__ double2 calcOutwardVector_Kernel(CellCuda* cellA, CellCuda* cellB, int2 const &size)
{
	ClusterCuda* clusterA = cellA->cluster;
	double2 posA = clusterA->pos;
	double2 velA = clusterA->vel;
	double angVelA = clusterA->angularVel * DEG_TO_RAD;

	ClusterCuda* clusterB = cellB->cluster;
	double2 posB = clusterB->pos;
	double2 velB = clusterB->vel;
	double angVelB = clusterB->angularVel * DEG_TO_RAD;

	double2 rAPp = sub(cellB->absPos, posA);
	mapDisplacementCorrection_Kernel(rAPp, size);
	rotateQuarterCounterClockwise_Kernel(rAPp);
	double2 rBPp = sub(cellB->absPos, posB);
	mapDisplacementCorrection_Kernel(rBPp, size);
	rotateQuarterCounterClockwise_Kernel(rBPp);
	return sub(sub(velB, mul(rBPp, angVelB)), sub(velA, mul(rAPp, angVelA)));
}

__device__ __inline__ void updateCollisionData_Kernel(int2 posInt, CellCuda *cell, CellCuda ** map
	, int2 const &size, CollisionData &collisionData)
{
	auto mapCell = getCellFromMap(posInt, map, size);
	if (mapCell != nullptr) {
		ClusterCuda* mapCluster = mapCell->cluster;
		if (mapCluster != cell->cluster) {
			if (mapDistanceSquared_Kernel(cell->absPos, mapCell->absPos, size) < CELL_MAX_DISTANCE*CELL_MAX_DISTANCE) {

				CollisionEntry* entry = collisionData.getOrCreateEntry(mapCluster);

				atomicAdd(&entry->numCollisions, 1);
				atomicAdd(&entry->collisionPos.x, mapCell->absPos.x);
				atomicAdd(&entry->collisionPos.y, mapCell->absPos.y);
				double2 outward = calcOutwardVector_Kernel(cell, mapCell, size);
				double2 n = calcNormalToCell_Kernel(mapCell, outward);
				atomicAdd(&entry->normalVec.x, n.x);
				atomicAdd(&entry->normalVec.y, n.y);

//				calcCollision_Kernel(mapCluster, mapCell, collisionData, size, false);
//				calcCollision_Kernel(mapCell->cluster, cell, collisionData, size, true);
			}
		}
	}
}

__device__ __inline__ void collectCollisionData_Kernel(CudaData const &data, CellCuda *cell, CollisionData &collisionData)
{
	auto absPos = cell->absPos;
	auto map = data.map1;
	auto size = data.size;
	int2 posInt = { static_cast<int>(absPos.x) , static_cast<int>(absPos.y) };
	if (!isCellPresentAtMap_Kernel(posInt, cell, map, size)) {
		return;
	}

	--posInt.x;
	--posInt.y;
	updateCollisionData_Kernel(posInt, cell, map, size, collisionData);
	++posInt.x;
	updateCollisionData_Kernel(posInt, cell, map, size, collisionData);
	++posInt.x;
	updateCollisionData_Kernel(posInt, cell, map, size, collisionData);

	++posInt.y;
	posInt.x -= 2;
	updateCollisionData_Kernel(posInt, cell, map, size, collisionData);
	posInt.x += 2;
	updateCollisionData_Kernel(posInt, cell, map, size, collisionData);

	++posInt.y;
	posInt.x -= 2;
	updateCollisionData_Kernel(posInt, cell, map, size, collisionData);
	++posInt.x;
	updateCollisionData_Kernel(posInt, cell, map, size, collisionData);
	++posInt.x;
	updateCollisionData_Kernel(posInt, cell, map, size, collisionData);
}
