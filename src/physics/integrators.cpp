
#include "integrators.h"
#include "collision_manager.h"
#include "optimizers.h"
#include "energy_calculator.h"
#include <Eigen/Dense>
using namespace std;
using Eigen::Matrix3Xd, Eigen::SparseMatrix;

// https://phys-sim-book.github.io/lec1.5-implicit_time_integration.html
void BackwardsEulerIntegrator::step() {
	Matrix3Xd originalPositions = state.positions;

	predictedPosition = state.positions + params.dt * state.velocities;
	optimizer->optimize();
	state.velocities = (state.positions - originalPositions) / params.dt;
}
double BackwardsEulerIntegrator::value() {
	return energyCalculator.inertiaValue(predictedPosition) + params.dt*params.dt*energyCalculator.potentialValue();
}
Matrix3Xd BackwardsEulerIntegrator::gradient() {
	return energyCalculator.inertiaGradient(predictedPosition) + params.dt*params.dt*energyCalculator.potentialGradient();
}
SparseMatrix<double> BackwardsEulerIntegrator::hessian() {
	return energyCalculator.inertiaHessian(predictedPosition) + params.dt*params.dt*energyCalculator.potentialHessian();
}

// https://en.wikipedia.org/wiki/Trapezoidal_rule_(differential_equations)
void TrapezoidalIntegrator::step() {
	Matrix3Xd originalPositions = state.positions;
	Matrix3Xd originalVelocities = state.velocities;

	predictedPosition = state.positions + (params.dt*state.velocities) + (params.dt*params.dt* -energyCalculator.potentialGradient());
	optimizer->optimize();
	state.velocities = 2*(state.positions - originalPositions) / params.dt - originalVelocities;
}
double TrapezoidalIntegrator::value() {
	return energyCalculator.inertiaValue(predictedPosition) + params.dt*params.dt*energyCalculator.potentialValue()/4;
}
Matrix3Xd TrapezoidalIntegrator::gradient() {
	return energyCalculator.inertiaGradient(predictedPosition) + params.dt*params.dt*energyCalculator.potentialGradient()/4;
}
SparseMatrix<double> TrapezoidalIntegrator::hessian() {
	return energyCalculator.inertiaHessian(predictedPosition) + params.dt*params.dt*energyCalculator.potentialHessian()/4;
}





