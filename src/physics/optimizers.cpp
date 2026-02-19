
#include "optimizers.h"
#include "integrators.h"
#include <iostream>
#include <Eigen/Dense>
#include <Eigen/Sparse>

using namespace std;
using Eigen::Matrix3Xd, Eigen::SparseMatrix, Eigen::VectorXd, Eigen::Vector3d; 

void applyDBC(Matrix3Xd& gradient, const vector<bool>& isDBC) {
	for (int vidx=0; vidx<gradient.cols(); vidx++) {
		if (isDBC[vidx]) {
			gradient.col(vidx) = Vector3d(0, 0, 0);
		}
	}
}

void applyDBC(SparseMatrix<double>& hessian, const vector<bool>& isDBC) {
	for (int col=0; col<hessian.outerSize(); col++) {
        for (SparseMatrix<double>::InnerIterator it(hessian, col); it; ++it) {
			
			int row = it.row();
			if (isDBC[(int) row / 3] || isDBC[(int) col / 3]) {
				it.valueRef() = row == col ? 1 : 0;
			}
        }
    }
}





Matrix3Xd NewtonOptimizer::getSearchDirection() {
	Matrix3Xd grad = integrator->gradient();
	SparseMatrix<double> hess = integrator->hessian();

	applyDBC(grad, state.isDBC);
	applyDBC(hess, state.isDBC);

	Eigen::ConjugateGradient<SparseMatrix<double>> solver;
	solver.compute(hess);

    if (solver.info() != Eigen::Success) {
        cerr << "Solver failed to compute decompose Hessian!\n";
        return Matrix3Xd::Zero(3, state.numPoints);
    }
	
	VectorXd searchDirection = solver.solve(-Eigen::Map<VectorXd>(grad.data(), 3*state.numPoints));
	return Eigen::Map<Matrix3Xd>(searchDirection.data(), 3, state.numPoints);
}
void NewtonOptimizer::optimize() {
	
	// Run broadphase and calculate full distance value/grad/hess of all valid collision pairs
	integrator->collisionManager.broadPhase();
	integrator->collisionManager.updateActivePairs(false);

	Matrix3Xd previousPositions = state.positions;
	Matrix3Xd searchDirection = getSearchDirection();
	double IP = integrator->value();

	int iter = 0;
	while (searchDirection.colwise().lpNorm<1>().maxCoeff() / params.dt > params.tolerance)  {
		if (iter++ > params.maxIter) break;

		// Line search to guarantees a step size that reduces the systems energy
		double alpha = integrator->collisionManager.CCD(searchDirection);
		state.positions = previousPositions + alpha*searchDirection;
		
		integrator->collisionManager.updateActivePairs(true);
		double newIP = integrator->value();

		int lsIter = 0;
		while (newIP > IP) {
			if (lsIter++ > params.lsMaxIter) break;

			alpha *= params.lsContraction;

			state.positions = previousPositions + alpha*searchDirection;
			integrator->collisionManager.updateActivePairs(true);
			newIP = integrator->value();

			if (alpha < params.lsLowerBound) break;
		}
		
		// Update IP & calculate next search direction
		IP = newIP;
		integrator->collisionManager.updateActivePairs(false);
		searchDirection = getSearchDirection();
		previousPositions = state.positions;
	}
}




Matrix3Xd LBFGSOptimizer::getSearchDirection() {
	//! TODO
}
void LBFGSOptimizer::optimize() {
	//! TODO
}

