
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

void Optimizer::lineSearch(Matrix3Xd& searchDirection) {

	// Record initial positions and total energy
	Matrix3Xd initialPositions = state.positions;
	double initialEnergy = integrator->value();

	// State initial step and record energy
	double alpha = integrator->collisionManager.CCD(searchDirection);
	state.positions = initialPositions + alpha * searchDirection;
	integrator->collisionManager.updateActivePairs(D_VALUE);
	double finalEnergy = integrator->value();

	// Contract step size until final energy < initial energy
	int iter = 0;	
	while (iter++ < params.lsMaxIter && finalEnergy > initialEnergy && alpha > params.lsLowerBound) {

		alpha *= params.lsContraction;
		state.positions = initialPositions + alpha*searchDirection;

		integrator->collisionManager.updateActivePairs(D_VALUE);
		finalEnergy = integrator->value();		
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
	
	integrator->collisionManager.broadPhase();
	integrator->collisionManager.updateActivePairs(D_VALUE | D_GRAD | D_HESS);
	Matrix3Xd searchDirection = getSearchDirection();

	int iter = 0;
	while (iter++ < params.maxIter && searchDirection.colwise().lpNorm<1>().maxCoeff() / params.dt > params.tolerance)  {
		lineSearch(searchDirection);

		integrator->collisionManager.updateActivePairs(D_VALUE | D_GRAD | D_HESS);
		searchDirection = getSearchDirection();
	}
}




// https://apxml.com/courses/optimization-techniques-ml/chapter-2-second-order-optimization-methods/l-bfgs-algorithm
Matrix3Xd LBFGSOptimizer::getSearchDirection(Eigen::Matrix3Xd& gradient) {
	
	int curHistorySize = (int) positionChangeHistory.size();
	
	// Use A = I initial approximation
	if (curHistorySize == 0) {
		return -gradient;

	// Use A = gamma*I where gamma uses the most recent s and y (s.y/y.y)
	} else {

		VectorXd q = Eigen::Map<VectorXd>(gradient.data(), 3*state.numPoints);
		vector<double> rhos(curHistorySize);
		vector<double> alphas(curHistorySize);

		// Backward pass
		for (int k=curHistorySize-1; k>=0; k--) {
			const VectorXd& s = positionChangeHistory[k];
			const VectorXd& y = gradientChangeHistory[k];

			double rho = 1/s.dot(y);
			double alpha = rho*s.dot(q);

			rhos[k] = rho;
			alphas[k] = alpha;

			q = q - alpha * y;
		}

		// Scaling
		q *= positionChangeHistory.back().dot(gradientChangeHistory.back()) / gradientChangeHistory.back().dot(gradientChangeHistory.back());
		
		// Forward pass
		for (int k=0; k<curHistorySize; k++) {
			const VectorXd& s = positionChangeHistory[k];
			const VectorXd& y = gradientChangeHistory[k];
			
			q += (alphas[k] - rhos[k]*y.dot(q)) * s;
		}

		q *= -1;
		return Eigen::Map<Matrix3Xd>(q.data(), 3, state.numPoints);
	}
}

void LBFGSOptimizer::optimize() {

	// Clear history and run broadphase
	reset();
	integrator->collisionManager.broadPhase();
	integrator->collisionManager.updateActivePairs(D_VALUE | D_GRAD);

	// Calculate initial position and gradient
	Matrix3Xd currentPosition = state.positions;
	Matrix3Xd currentGradient = integrator->gradient();
	applyDBC(currentGradient, state.isDBC);


	for (int iter=0; iter < params.maxIter; iter++) {
	
		// Calculate search direction and apply line search
		Matrix3Xd searchDirection = getSearchDirection(currentGradient);
		lineSearch(searchDirection); 
		
		// Recalculate position and gradient
		integrator->collisionManager.updateActivePairs(D_VALUE | D_GRAD);
		Matrix3Xd newPosition = state.positions;
		Matrix3Xd newGradient = integrator->gradient();
		applyDBC(newGradient, state.isDBC);

		// Record change in position and change in gradient
		updateHistory(currentPosition, newPosition, currentGradient, newGradient);

		// Update current values
		currentPosition = newPosition;
		currentGradient = newGradient;

		// Check for convergence
		if (currentGradient.cwiseAbs().maxCoeff() < params.tolerance) break;
	}
}

void LBFGSOptimizer::updateHistory(Matrix3Xd& initialPosition, Matrix3Xd& finalPosition, Matrix3Xd& initialGradient, Matrix3Xd& finalGradient) {

	Matrix3Xd positionChange = finalPosition - initialPosition;
	Matrix3Xd gradientChange = finalGradient - initialGradient;

	auto s = Eigen::Map<VectorXd>(positionChange.data(), 3*state.numPoints);
	auto y = Eigen::Map<VectorXd>(gradientChange.data(), 3*state.numPoints);

	if (y.dot(s) > 1e-10 * s.norm() * y.norm()) {
		positionChangeHistory.push_back(s);
		gradientChangeHistory.push_back(y);

		if ((int) positionChangeHistory.size() > maxHistorySize) {
			positionChangeHistory.erase(positionChangeHistory.begin());
			gradientChangeHistory.erase(gradientChangeHistory.begin());
		}
	}
}

void LBFGSOptimizer::reset() {
	positionChangeHistory.clear(); 
	gradientChangeHistory.clear(); 
}

