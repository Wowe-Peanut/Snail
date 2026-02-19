#pragma once

#include "mesh.h"
#include "distances.h"
#include <Eigen/Dense>

struct SimParameters;
struct SimState;

struct CollisionPair {
	Distance dist;
	double contactArea;

	virtual void update(SimState& state, bool valueOnly) = 0;
	virtual double CCD(SimParameters& params, SimState& state, Eigen::Matrix3Xd& searchDirection) = 0;
	virtual std::vector<int> getDofIdxs() = 0;
};

struct PointTriangle : CollisionPair {
	int p, t1, t2, t3;

	PointTriangle(int p, int t1, int t2, int t3): p(p), t1(t1), t2(t2), t3(t3) {};
	void update(SimState& state, bool valueOnly) override;
	double CCD(SimParameters& params, SimState& state, Eigen::Matrix3Xd& searchDirection) override;
	std::vector<int> getDofIdxs() { return {p, t1, t2, t3}; }
};

struct EdgeEdge : CollisionPair {
	int e1, e2, e3, e4;

	EdgeEdge(int e1, int e2, int e3, int e4): e1(e1), e2(e2), e3(e3), e4(e4) {};
	void update(SimState& state, bool valueOnly) override;
	double CCD(SimParameters& params, SimState& state, Eigen::Matrix3Xd& searchDirection) override;
	std::vector<int> getDofIdxs() { return {e1, e2, e3, e4}; }
};

struct CollisionManager {
	SimParameters& params;
	SimState& state;

	CollisionManager(SimParameters& params, SimState& state): params(params), state(state) {};
	void broadPhase();
	void trianglesToCollisionPairs(const Triangle& tri1, const Triangle& tri2);
	double CCD(Eigen::Matrix3Xd& searchDirection);
	void updateActivePairs(bool valueOnly);
};