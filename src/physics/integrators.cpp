
#include "integrators.h"
#include "collision_manager.h"
#include "optimizers.h"
#include "energy_calculator.h"
#include <Eigen/Dense>
using namespace std;
using Eigen::Matrix3Xd, Eigen::SparseMatrix;

void ImplicitIntegrator::reset() {
	optimizer->reset(); 
}


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
// !NOT WORKING AT THE MOMENT!
void TrapezoidalIntegrator::step() {

}
double TrapezoidalIntegrator::value() {

}
Matrix3Xd TrapezoidalIntegrator::gradient() {

}
SparseMatrix<double> TrapezoidalIntegrator::hessian() {

}





