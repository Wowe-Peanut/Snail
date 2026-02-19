
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
	while (iter++ > params.lsMaxIter && finalEnergy > initialEnergy && alpha > params.lsLowerBound) {

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
Matrix3Xd LBFGSOptimizer::getSearchDirection() {
	
	int curHistorySize = (int) positionChangeHistory.size();
	Matrix3Xd grad = integrator->gradient();
	lastGradientCalculated = grad;

	// Use A = I initial approximation
	if (curHistorySize == 0) {
		return -grad;

	// Use A = gamma*I where gamma uses the most recent s and y (s.y/y.y)
	} else {

		VectorXd q = Eigen::Map<VectorXd>(grad.data(), 3*state.numPoints);
		vector<double> rhos(curHistorySize);
		vector<double> alphas(curHistorySize);

		// Backward pass
		for (int k=curHistorySize-1; k>=0; k--) {
			const Vector3d& s = positionChangeHistory[k];
			const Vector3d& y = gradientChangeHistory[k];

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
			const Vector3d& s = positionChangeHistory[k];
			const Vector3d& y = gradientChangeHistory[k];
			
			q += (alphas[k] - rhos[k]*y.dot(q)) * s;
		}

		q *= -1;
		return Eigen::Map<Matrix3Xd>(q.data(), 3, state.numPoints);
	}
}

void LBFGSOptimizer::optimize() {
	integrator->collisionManager.broadPhase();
	integrator->collisionManager.updateActivePairs(D_VALUE | D_GRAD);
	Matrix3Xd searchDirection = getSearchDirection();

	int iter = 0;
	while (iter++ < params.maxIter && searchDirection.colwise().lpNorm<1>().maxCoeff() / params.dt > params.tolerance)  {

		Matrix3Xd initalPositions = state.positions;
		Matrix3Xd initialGradient = lastGradientCalculated;

		lineSearch(searchDirection);
		integrator->collisionManager.updateActivePairs(D_VALUE | D_GRAD);
		searchDirection = getSearchDirection();

		Matrix3Xd positionChange = state.positions - initalPositions;
		Matrix3Xd gradientChange = lastGradientCalculated - initialGradient;
		updateHistory(positionChange, gradientChange);
	}
}

void LBFGSOptimizer::updateHistory(Matrix3Xd& positionChange, Matrix3Xd& gradientChange) {
	auto s = Eigen::Map<Vector3d>(positionChange.data(), 3*state.numPoints);
	auto y = Eigen::Map<Vector3d>(gradientChange.data(), 3*state.numPoints);

	if (s.dot(y) > 0.0) {
		positionChangeHistory.push_back(s);
		gradientChangeHistory.push_back(y);

		if ((int) positionChangeHistory.size() > maxHistorySize) {
			positionChangeHistory.erase(positionChangeHistory.begin());
			gradientChangeHistory.erase(gradientChangeHistory.begin());
		}
	}
}

