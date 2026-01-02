
#include "time_integrator.h"
#include <iostream>
using namespace std;
using Eigen::Matrix3Xd, Eigen::SparseMatrix, Eigen::VectorXd, Eigen::Vector3d; 

Matrix3Xd TimeIntegrator::getSearchDirection(Matrix3Xd& xtilde) {
	SparseMatrix<double> hess = energyCalculator.IPHessian(xtilde);
	Matrix3Xd grad = energyCalculator.IPGradient(xtilde);

	// Gradient sticky DBCs
	for (int vidx=0; vidx<state.numPoints; vidx++) {
		if (state.isDBC[vidx]) {
			grad.col(vidx) = Vector3d(0, 0, 0);
		}
	}

	// Hess sticky DBCs
    for (int col=0; col<hess.outerSize(); col++) {
        for (SparseMatrix<double>::InnerIterator it(hess, col); it; ++it) {
			
			int row = it.row();
			if (state.isDBC[(int) row / 3] || state.isDBC[(int) col / 3]) {
				it.valueRef() = row == col ? 1 : 0;
			}
        }
    }
 
	// Sparse solver 
	Eigen::SparseLU<SparseMatrix<double>> solver;
	solver.compute(hess);

    if (solver.info() != Eigen::Success) {
        cerr << "Solver failed to compute decompose Hessian!\n";
        return Matrix3Xd::Zero(3, state.numPoints);
    }

	VectorXd p = solver.solve(-Eigen::Map<VectorXd>(grad.data(), 3*state.numPoints));
	return Eigen::Map<Matrix3Xd>(p.data(), 3, state.numPoints);
}

// Step functions
void TimeIntegrator::step() {
	if (state.objects.empty()) return;	

	// Read from state
	Matrix3Xd& positions = state.positions;
	Matrix3Xd& velocities = state.velocities;
	double dt = params.dt;
	double tol = params.tolerance;

	Matrix3Xd originalPositions = positions; // Before any newton iteration
	Matrix3Xd previousPositions = positions; // From previous newton iteration
	
	// Calculate initial Incremental Potential value and search direction 
	Matrix3Xd predictedPositions = positions + dt*velocities;
	Matrix3Xd searchDirection = getSearchDirection(predictedPositions);
	double IP = energyCalculator.IPValue(predictedPositions);

	// Projected Newton Loop
	while (searchDirection.colwise().lpNorm<1>().maxCoeff() / dt > tol)  {

		// Line search to guarantees a step size that reduces the systems energy
		double alpha = collisionManager.CCD(searchDirection);
		positions = previousPositions + alpha*searchDirection;

		double newIP = energyCalculator.IPValue(predictedPositions);

		while (newIP > IP) {
			alpha /= 2;
			positions = previousPositions + alpha*searchDirection;
			newIP = energyCalculator.IPValue(predictedPositions);

			if (alpha > ALPHA_LOWER_BOUND) break;
		}
		
		// Update IP & calculate next search direction
		IP = newIP;
		searchDirection = getSearchDirection(predictedPositions);
		previousPositions = positions;
	}

	// Update velocities with final positions
	velocities = (positions - originalPositions) / dt;
}