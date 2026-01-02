#pragma once

#include "energy_calculator.h"
#include "collision_manager.h"
#include "physics_engine.h"
#include "object.h"
#include <Eigen/Dense>

const double ALPHA_LOWER_BOUND = 0.00001;

struct TimeIntegrator {
	SimParameters& params;
	SimState& state;

	EnergyCalculator energyCalculator;
	CollisionManager collisionManager;

	TimeIntegrator(SimParameters& params, SimState& state): 
		params(params), state(state), energyCalculator(params, state), collisionManager(params, state) {};

	void step();
    Eigen::Matrix3Xd getSearchDirection(Eigen::Matrix3Xd& xtilde);
};