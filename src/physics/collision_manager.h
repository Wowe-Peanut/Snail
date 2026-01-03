#pragma once

#include "mesh.h"
#include <Eigen/Dense>

struct SimParameters;
struct SimState;

struct CollisionPair {
	double dvalue;
	Eigen::Matrix3Xd dgrad;
	Eigen::MatrixXd dhess;

	virtual void updateValue() = 0;
	virtual void updateGrad() = 0;
	virtual void updateHess() = 0;
};

struct PointTriangle : CollisionPair {
	int p, t1, t2, t3;
	void updateValue() override;
	void updateGrad() override;
	void updateHess() override;
};

struct EdgeEdge : CollisionPair {
	int e1, e2, e3, e4;
	void updateValue() override;
	void updateGrad() override;
	void updateHess() override;
};

struct CollisionManager {
	SimParameters& params;
	SimState& state;

	CollisionManager(SimParameters& params, SimState& state): params(params), state(state) {};
	void broadphase();
	double CCD(Eigen::Matrix3Xd& searchDirection);
	void updateActivePairs(bool updateValues, bool updateGradients, bool updateHessians);
};