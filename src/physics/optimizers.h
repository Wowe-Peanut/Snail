#pragma once

#include "integrators.h"
#include "physics_engine.h"
#include <vector>
#include <Eigen/Dense>
#include <Eigen/Sparse>


// Helper functions
void applyDBC(Eigen::Matrix3Xd& gradient, const std::vector<bool>& isDBC);
void applyDBC(Eigen::SparseMatrix<double>& hessian, const std::vector<bool>& isDBC);


struct ImplicitIntegrator;

struct Optimizer {
    SimParameters& params;
	SimState& state;
    ImplicitIntegrator* integrator; // weak to avoid circular reference between an integrator and its optimizer

    Optimizer(SimParameters& params, SimState& state): params(params), state(state) {};
    void lineSearch(Eigen::Matrix3Xd& searchDirection);

    virtual Eigen::Matrix3Xd getSearchDirection() = 0;
    virtual void optimize() = 0;
};

struct NewtonOptimizer : Optimizer {
    NewtonOptimizer(SimParameters& params, SimState& state): Optimizer(params, state) {};
    void optimize() override;
    Eigen::Matrix3Xd getSearchDirection() override;
};

struct LBFGSOptimizer : Optimizer {
    int maxHistorySize;

    std::vector<Eigen::VectorXd> positionChangeHistory;
    std::vector<Eigen::VectorXd> gradientChangeHistory;
    
    LBFGSOptimizer(SimParameters& params, SimState& state, int maxHistorySize): 
        Optimizer(params, state), maxHistorySize(maxHistorySize) {};

    void optimize() override;
    Eigen::Matrix3Xd getSearchDirection() override;
};

