
#include "energy_calculator.h"
#include "physics_engine.h"
using namespace std;
using Eigen::Matrix3Xd, Eigen::SparseMatrix, Eigen::VectorXd, Eigen::Vector3d, Eigen::MatrixXd, Eigen::Matrix3d, Eigen::Triplet; 

void EnergyCalculator::makePSD(MatrixXd& mat) {

	// Self-adjoint (A = A^T) matrix has real eigenvalues and orthogonal eigenvectors and our local hess
	// is a block of (H, -H; -H, H) which is self-adjoint so we can use the SelfAdjointEigenSolver)
	Eigen::SelfAdjointEigenSolver<MatrixXd> es(mat);
	VectorXd evals = es.eigenvalues();
	MatrixXd evecs = es.eigenvectors();

	// Zero out negative eigenvalues to make PSD
	for (int i=0; i<evals.size(); i++) {
		if (evals(i) < 0) evals(i) = 0;
	}

	// Reconstruct matrix with new eigenvalues
	mat = evecs * evals.asDiagonal() * evecs.transpose();
}

// Incremental Potential Energy
double EnergyCalculator::ipValue(Matrix3Xd& xtilde) {
	double dt = params.dt;
	return inertiaValue(xtilde) + dt*dt*(massSpringValue() + gravityValue() + contactValue());
}
Matrix3Xd EnergyCalculator::ipGradient(Matrix3Xd& xtilde) {
	double dt = params.dt;
	return inertiaGradient(xtilde) + dt*dt*(massSpringGradient() + gravityGradient() + contactGradient());
}
SparseMatrix<double> EnergyCalculator::ipHessian(Matrix3Xd& xtilde) {
	double dt = params.dt;
	return inertiaHessian(xtilde) + dt*dt*(massSpringHessian() + contactHessian());
}



// Inertia Energy 
double EnergyCalculator::inertiaValue(Matrix3Xd& xtilde) {
	double sum = 0;
	for (int vidx=0; vidx<state.numPoints; vidx++) {
		Vector3d diff = state.positions.col(vidx) - xtilde.col(vidx);
		sum += diff.dot(diff);
	}

	return params.pointMass * sum / 2;
}
Matrix3Xd EnergyCalculator::inertiaGradient(Matrix3Xd& xtilde) {
	return params.pointMass * (state.positions - xtilde);
}
SparseMatrix<double> EnergyCalculator::inertiaHessian(Matrix3Xd& xtilde) {

	// From eigen docs: "The cost of a single purely random insertion into a SparseMatrix is O(nnz), 
	// where nnz is the current number of non-zero coefficients."
	// So it recommends using triplets, which constructs the SparseMatrix in O(n) with n the number of triplets

	int dof = 3*state.numPoints;
	vector<Triplet<double>> triplets(dof);
	for (int i=0; i<dof; i++) {
		triplets[i] = Triplet<double>(i, i, params.pointMass);
	}

	SparseMatrix<double> hess(dof, dof);
	hess.setFromSortedTriplets(triplets.begin(), triplets.end());

	return hess;
}



// Mass Spring Energy 
double EnergyCalculator::massSpringValue() {
	double sum = 0;
	for (Edge& edge: state.edges) {
		Vector3d diff = state.positions.col(edge.v1) - state.positions.col(edge.v2);
		sum += edge.l2 * pow(diff.dot(diff) / edge.l2 - 1, 2);
	}
	return sum * params.springStiffness / 2;
}
Matrix3Xd EnergyCalculator::massSpringGradient() {
	Matrix3Xd grad = MatrixXd::Zero(3, state.numPoints);

	for (Edge& edge: state.edges) {
		Vector3d diff = state.positions.col(edge.v1) - state.positions.col(edge.v2);
		Vector3d edgeGrad = 2 * params.springStiffness * (diff.dot(diff) / edge.l2 - 1) * diff;

		grad.col(edge.v1) += edgeGrad;
		grad.col(edge.v2) -= edgeGrad;
	}

	return grad;
}
SparseMatrix<double> EnergyCalculator::massSpringHessian() {

	vector<Triplet<double>> triplets;
	triplets.reserve(36*state.edges.size()); // 1 edge = 2 vertices = 6 dof = 36 hessian entries

	for (Edge& edge: state.edges) {
		Vector3d diff = state.positions.col(edge.v1) - state.positions.col(edge.v2);

		// Hessian for the energy of single edge, 3x3 for each DIFFERENCE in the two vertices
		Matrix3d diffHess = 2 * params.springStiffness / edge.l2 * (2 * diff * diff.transpose() + (diff.dot(diff) - edge.l2) * Matrix3d::Identity());

		// Essemble 6x6 hessian for the 6 DOFs on the two vertices of the edge. diffHess is symmetric, so 
		// this block matrix will also be symmetric so we can use a SelfAdjointEigenSolver to make PSD
		MatrixXd localHess(6, 6);
		localHess.block<3,3>(0,0) = diffHess;
		localHess.block<3,3>(0,3) = -diffHess;
		localHess.block<3,3>(3,0) = -diffHess;
		localHess.block<3,3>(3,3) = diffHess;
		makePSD(localHess);


		for (int blockRow=0; blockRow<=1; blockRow++) {
			for (int blockCol=0; blockCol<=1; blockCol++) {

				int startRow = (blockRow == 0 ? 3*edge.v1 : 3*edge.v2);
				int startCol = (blockCol == 0 ? 3*edge.v1 : 3*edge.v2);

				for (int row=0; row<3; row++) {
					for (int col=0; col<3 ;col++) {
						double value = localHess(3*blockRow+row, 3*blockCol+col);

						triplets.push_back(Triplet<double>(startRow+row, startCol+col, value));
					}
				}	
			}
		}
	}

	SparseMatrix<double> hess = SparseMatrix<double>(3*state.numPoints, 3*state.numPoints);
	hess.setFromTriplets(triplets.begin(), triplets.end());
	return hess;
}



// Gravity Energy 
double EnergyCalculator::gravityValue() {
	double sum = 0;
	for (int vidx=0; vidx<state.numPoints; vidx++) {
		sum += params.gravity.dot(state.positions.col(vidx));
	}

	return -sum * params.pointMass;
}
Matrix3Xd EnergyCalculator::gravityGradient() {
	Matrix3Xd grad = Matrix3Xd::Zero(3, state.numPoints);
	for (int vidx=0; vidx<state.numPoints; vidx++) {
		grad.col(vidx) = -params.pointMass * params.gravity;
	}

	return grad;
}


// Contact energy
double EnergyCalculator::contactValue() {

	double sum = 0;
	for (shared_ptr<CollisionPair> cp: state.activeCollisionPairs) {
		if (cp->dist.value < params.contactDistance) {
			sum += 0.5 * cp->contactArea * barrier(cp->dist.value);
		}
	}	

	return sum;
}
Matrix3Xd EnergyCalculator::contactGradient() {

	Matrix3Xd grad = Matrix3Xd::Zero(3, state.numPoints);
	for (shared_ptr<CollisionPair> cp: state.activeCollisionPairs) {
		if (cp->dist.value < params.contactDistance) {
			
			Matrix3Xd localGrad = 0.5 * cp->contactArea * barrierD(cp->dist.value) * cp->dist.grad;

			vector<int> dofIdxs = cp->getDofIdxs();
			for (int i=0; i<dofIdxs.size(); i++) {
				grad.col(dofIdxs[i]) += localGrad.col(i);
			}
		}
	}	

	return grad;
}
SparseMatrix<double> EnergyCalculator::contactHessian() {

	vector<Triplet<double>> triplets;
	for (shared_ptr<CollisionPair> cp: state.activeCollisionPairs) {
		if (cp->dist.value < params.contactDistance) {
			
			Distance& d = cp->dist;
			Eigen::Map<VectorXd> flatgrad(d.grad.data(), 12);
			MatrixXd localHess = 0.5 * cp->contactArea * (barrierD2(d.value) * flatgrad * flatgrad.transpose() + barrierD(d.value) * d.hess);
			makePSD(localHess);

			// Map local hess to global triplets
			vector<int> dofIdxs = cp->getDofIdxs();
			for (int row=0; row<dofIdxs.size(); row++) {
				for (int col=0; col<dofIdxs.size(); col++) {

					Matrix3d submat = localHess.block<3, 3>(3*row, 3*col);
					for (int subrow=0; subrow<3; subrow++) {
						for (int subcol=0; subcol<3; subcol++) {
							double value = submat(subrow, subcol);

							triplets.push_back(Triplet<double>(3*dofIdxs[row] + subrow, 3*dofIdxs[col] + subcol, value));
						}
					}
				}
			}
		}
	}	

	SparseMatrix<double> hess = SparseMatrix<double>(3*state.numPoints, 3*state.numPoints);
	hess.setFromTriplets(triplets.begin(), triplets.end());
	return hess;
}

// d^2 Barrier energy
double EnergyCalculator::barrier(double d2) {
	double s = d2/(params.contactDistance * params.contactDistance);
	double beta = params.contactStiffness/8 * params.contactDistance;

	return beta*(s-1)*log(s);
}
double EnergyCalculator::barrierD(double d2) {
	double dhat2 = params.contactDistance * params.contactDistance;
	double s = d2/dhat2;
	double beta = params.contactStiffness/8 * params.contactDistance;

	return beta/dhat2*(log(s)+1-1/s);
}
double EnergyCalculator::barrierD2(double d2) {
	double dhat2 = params.contactDistance * params.contactDistance;
	double s = d2/dhat2;
	double beta = params.contactStiffness/8 * params.contactDistance;

	return beta/dhat2/dhat2*(s+1)/(s*s);
}
