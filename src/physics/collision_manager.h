#pragma once

#include "mesh.h"
#include "distances.h"
#include <Eigen/Dense>

struct SimParameters;
struct SimState;

struct CollisionPair {
	Distance dist;

	// Updates distance calculations (and grad/hess if specified)
	virtual void update(bool valueOnly) = 0;

	// 'Time of impact' lowerbound used for ACCD step size estimation
	virtual double toiLowerBound(Eigen::Matrix3Xd& searchDirection) = 0;
};

struct PointTriangle : CollisionPair {
	int p, t1, t2, t3;
	void update(bool valueOnly) override;
	double toiLowerBound(Eigen::Matrix3Xd& searchDirection) override;
};

struct EdgeEdge : CollisionPair {
	int e1, e2, e3, e4;
	void update(bool valueOnly) override;
	double toiLowerBound(Eigen::Matrix3Xd& searchDirection) override;
};

struct CollisionManager {
	SimParameters& params;
	SimState& state;
	std::vector<std::shared_ptr<CollisionPair>> activePairs;

	CollisionManager(SimParameters& params, SimState& state): params(params), state(state) {};
	void broadPhase();
	double CCD(Eigen::Matrix3Xd& searchDirection);
	void updateActivePairs(bool valueOnly);
};