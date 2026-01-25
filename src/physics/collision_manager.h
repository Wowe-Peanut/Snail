#pragma once

#include "mesh.h"
#include "distances.h"
#include <Eigen/Dense>

struct SimParameters;
struct SimState;

struct CollisionPair {
	Distance dist;
	virtual void update(bool valueOnly) = 0;
	virtual double CCD(Eigen::Matrix3Xd& searchDirection) = 0;
	virtual double contactArea() = 0;
	virtual std::vector<int> getDofIdxs() = 0;
}

struct PointTriangle {
	Distance dist;
	int p, t1, t2, t3;
	void update(bool valueOnly) override;
	double CCD(Eigen::Matrix3Xd& searchDirection) override;
	double contactArea() override;
	std::vector<int> getDofIdxs() override;
};

struct EdgeEdge {
	Distance dist;
	int e1, e2, e3, e4;
	void update(bool valueOnly) override;
	double CCD(Eigen::Matrix3Xd& searchDirection) override;
	double contactArea() override;
	std::vector<int> getDofIdxs() override;
};

struct CollisionManager {
	SimParameters& params;
	SimState& state;

	CollisionManager(SimParameters& params, SimState& state): params(params), state(state) {};
	void broadPhase();
	double CCD(Eigen::Matrix3Xd& searchDirection);
	void updateActivePairs(bool valueOnly);
};