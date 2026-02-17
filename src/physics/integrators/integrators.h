#pragma once

#include "collision_manager.h"
#include "optimizers.h"
#include "energy_calculator.h"
#include <Eigen/Dense>

struct SimParameters;
struct SimState;

struct Integrator {
    SimParameters& params;
	SimState& state;
    CollisionManager collisionManager;

    Integrator(SimParameters& params, SimState& state): params(params), state(state), collisionManager(params, state) {};
    virtual void step() = 0;
};  

struct ImplicitIntegrator : Integrator {
    EnergyCalculator energyCalculator;
    std::unique_ptr<Optimizer> optimizer;

    ImplicitIntegrator(SimParameters& params, SimState& state); // Needs to construct the appropriate optimizer based on the information in params
    virtual double value() = 0;
    virtual Eigen::Matrix3Xd gradient() = 0;
    virtual Eigen::SparseMatrix<double> hessian() = 0;
};

struct BackwardsEulerIntegrator : ImplicitIntegrator {
    double value() override;
    Eigen::Matrix3Xd gradient() override;
    Eigen::SparseMatrix<double> hessian() override;
};

