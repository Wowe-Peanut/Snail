#pragma once

#include "integrators.h"
#include <Eigen/Dense>

// Optimizer's takes an implicit integrator b/c they implement the object functions being minimized
struct Optimizer {
    SimParameters& params;
	SimState& state;
    ImplicitIntegrator* integrator;    
    virtual void solve() = 0;
};

struct LBFGSOptimizer : Optimizer {
    int historySize;
    std::list<Eigen::Matrix3Xd> positionChangeHistory;
    std::list<Eigen::Matrix3Xd> gradientChangeHistory;
    
    LBFGSOptimizer(SimParameters& params, SimState& state, ImplicitIntegrator* integrator, int historySize): params(params), state(state), integrator(integrator), historySize(historySize) {};
    void solve() override;
};

struct NewtonOptimizer : Optimizer {
    NewtonOptimizer(SimParameters& params, SimState& state, ImplicitIntegrator* integrator): params(params), state(state), integrator(integrator) {};
    void solve() override;
};