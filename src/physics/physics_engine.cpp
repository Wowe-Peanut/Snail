
#include "physics_engine.h"
#include <iostream>
#include <Eigen/Dense>
#include <Eigen/Sparse>
using namespace std;

PhysicsEngine::PhysicsEngine(vector<shared_ptr<Object>>& objects, SimParameters& params): params(params), integrator(params, state) {

    state.numPoints = 0;
    for (auto obj: objects) {	
		state.objects.push_back(obj);
		state.offsets.push_back(state.numPoints);
		state.numPoints += obj->mesh->numPoints;
    }

	
	// Initialize the ensemble state of all objects
	state.positions = Eigen::Matrix3Xd::Zero(3, state.numPoints);
	state.velocities = Eigen::Matrix3Xd::Zero(3, state.numPoints); 

	for (size_t objIdx=0; objIdx<objects.size(); objIdx++) {
		auto obj = objects[objIdx];
		int offset = state.offsets[objIdx];

		// Copy positions
		Eigen::Map<Eigen::Matrix3Xf> floatMatrix(obj->mesh->triPosBuf.data(), 3, obj->mesh->numPoints);
		state.positions.middleCols(offset, obj->mesh->numPoints) = floatMatrix.cast<double>();
		
		// Copy edges
		for (Edge& edge: obj->mesh->edges) {
			state.edges.push_back({offset+edge.v1, offset+edge.v2, edge.l2});
		}
		
		// Copy surface primitives
		for (Triangle& tri: obj->mesh->triangles) {
			state.triangles.emplace_back(offset+tri.v1, offset+tri.v2, offset+tri.v3);
		}
		
		// Copy fixed points
		state.isDBC.insert(state.isDBC.end(), obj->mesh->isFixedPoint.begin(), obj->mesh->isFixedPoint.end());

		// Copy velocity
		Eigen::Vector3d initialVelocity(obj->mesh->initialVelocity.x, obj->mesh->initialVelocity.y, obj->mesh->initialVelocity.z);
		for (int vidx=0; vidx < obj->mesh->numPoints; vidx++) {
			if (!obj->mesh->isFixedPoint[vidx]) {
				state.velocities.col(offset+vidx) = initialVelocity;
			}
		}

	}

	// each pairs of triangles (assuming no overlap) generates 6 point-triangle and 9 edge-edge collision pairs, so reserve enough space at the start to prevent resizing later
	int numTriangles = state.triangles.size();
	state.activeCollisionPairs.reserve(15*numTriangles*numTriangles);

	initialPositions = state.positions;
	initialVelocities = state.velocities;
}

void PhysicsEngine::step() {
	integrator.step();
	updateObjects();
}

void PhysicsEngine::reset() {
	state.positions = initialPositions;
	state.velocities = initialVelocities;
	updateObjects();
}

void PhysicsEngine::updateObjects() {
	for (size_t objIdx=0; objIdx<state.objects.size(); objIdx++) {
		auto obj = state.objects[objIdx];

		if (!obj->mesh->isStatic) {
			
			int offset = state.offsets[objIdx];
	
			Eigen::Matrix3Xf submatrix = state.positions.middleCols(offset, obj->mesh->numPoints).cast<float>();
			obj->mesh->triPosBuf.assign(submatrix.data(), submatrix.data() + submatrix.size());
			obj->mesh->computeSurfaceQualities();
		}
	}
}




