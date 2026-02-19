#pragma once

#include "integrators.h"
#include "physics_engine.h"
#include <list>
#include <Eigen/Dense>
#include <Eigen/Sparse>


// Helper functions
void applyDBC(Eigen::Matrix3Xd& gradient, const std::vector<bool>& isDBC);
void applyDBC(Eigen::SparseMatrix<double>& hessian, const std::vector<bool>& isDBC);
// void lineSearch();

struct ImplicitIntegrator;

struct Optimizer {
    SimParameters& params;
	SimState& state;
    ImplicitIntegrator* integrator; // weak to avoid circular reference between an integrator and its optimizer

    Optimizer(SimParameters& params, SimState& state): params(params), state(state) {};
    virtual Eigen::Matrix3Xd getSearchDirection() = 0;
    virtual void optimize() = 0;
};

struct NewtonOptimizer : Optimizer {
    NewtonOptimizer(SimParameters& params, SimState& state): Optimizer(params, state) {};
    void optimize() override;
    Eigen::Matrix3Xd getSearchDirection() override;
};

struct LBFGSOptimizer : Optimizer {
    int historySize;
    std::list<Eigen::Matrix3Xd> positionChangeHistory;
    std::list<Eigen::Matrix3Xd> gradientChangeHistory;
    
    LBFGSOptimizer(SimParameters& params, SimState& state, int historySize): Optimizer(params, state), historySize(historySize) {};
    void optimize() override;
    Eigen::Matrix3Xd getSearchDirection() override;
};

