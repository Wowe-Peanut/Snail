#pragma once

#include "collision_manager.h"
#include "optimizers.h"
#include "energy_calculator.h"
#include <Eigen/Dense>

struct SimParameters;
struct SimState;
struct Optimizer;

struct Integrator {
    SimParameters& params;
	SimState& state;
    CollisionManager collisionManager;

    Integrator(SimParameters& params, SimState& state): params(params), state(state), collisionManager(params, state) {};
    virtual void step() = 0;
};  

struct ImplicitIntegrator : Integrator {
    EnergyCalculator energyCalculator;
    std::shared_ptr<Optimizer> optimizer;

    ImplicitIntegrator(SimParameters& params, SimState& state): Integrator(params, state), energyCalculator(params, state) {};
    virtual double value() = 0;
    virtual Eigen::Matrix3Xd gradient() = 0;
    virtual Eigen::SparseMatrix<double> hessian() = 0;
};

struct BackwardsEulerIntegrator : ImplicitIntegrator {
    Eigen::Matrix3Xd predictedPosition; 

    BackwardsEulerIntegrator(SimParameters& params, SimState& state): ImplicitIntegrator(params, state) {};
    double value() override;
    Eigen::Matrix3Xd gradient() override;
    Eigen::SparseMatrix<double> hessian() override;
    void step() override;
};

struct TrapezoidalIntegrator : ImplicitIntegrator {
    Eigen::Matrix3Xd predictedPosition; 

    TrapezoidalIntegrator(SimParameters& params, SimState& state): ImplicitIntegrator(params, state) {};
    double value() override;
    Eigen::Matrix3Xd gradient() override;
    Eigen::SparseMatrix<double> hessian() override;
    void step() override;
};

