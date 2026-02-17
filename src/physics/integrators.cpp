
#include "integrators.h"
#include "collision_manager.h"
#include "optimizers.h"
#include "energy_calculator.h"
#include <Eigen/Dense>
using namespace std;
using Eigen::Matrix3Xd, Eigen::SparseMatrix;



ImplicitIntegrator::ImplicitIntegrator(SimParameters& params, SimState& state): Integrator(params, state) {
	//! TODO - should construct the optimizer based on 'params'
}

void BackwardsEulerIntegrator::step() {
	//! TODO - Should assign xtilde, call optimizer, then update velocity
}

double BackwardsEulerIntegrator::value() {
	//! TODO
}

Matrix3Xd BackwardsEulerIntegrator::gradient() {
	//! TODO
}

SparseMatrix<double> BackwardsEulerIntegrator::hessian() {
	//! TODO
}
