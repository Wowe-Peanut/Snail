#include "PhysicsEngine.h"
#include <iostream>

// #define EIGEN_DONT_ALIGN_STATICALLY
#include <Eigen/Dense>
#include <Eigen/Sparse>

using json = nlohmann::json;
using namespace std;
using Eigen::Vector3f, Eigen::Matrix3Xf, Eigen::VectorXf, Eigen::MatrixXf, Eigen::Matrix3f, Eigen::SparseMatrix, Eigen::Triplet;


PhysicsEngine::PhysicsEngine(vector<shared_ptr<Object>>& objectList, json parameters) {
    // Read parameters
    h                   = parameters["delta_time"];
    tol                 = parameters["tolerance"];
    maxiter             = parameters["max_iterations"];
    springStiffness     = parameters["spring_stiffness"];
    pointMass           = parameters["point_mass"];
    gravity             = Vector3f(0, parameters["gravity"], 0);

    // A running total used to calculate object indices (3*numpoints)
    numPoints = 0;
    for (auto obj: objectList) {
        if (obj->physicsObject) {
            physicsObjects.push_back(obj);
            objectOffsets.push_back(numPoints);
            numPoints += obj->numPoints;
        }
    }

	if (physicsObjects.empty()) {
		cerr << "WARNING | None of the constructed objects are set to be physicsObjects..." << endl;

	} else {
		// Initialize the ensemble state of all objects
		positions = Matrix3Xf::Zero(3, numPoints);
		velocities = Matrix3Xf::Zero(3, numPoints); 
		isFixedPoint = vector<bool>(numPoints, false);

		numEdges = 0;
		for (size_t objIdx=0; objIdx<physicsObjects.size(); objIdx++) {
			auto obj = physicsObjects[objIdx];
			int offset = objectOffsets[objIdx];

			positions.middleCols(offset, obj->numPoints) = Eigen::Map<Matrix3Xf>(obj->shape->posBuf.data(), 3, obj->numPoints);
			
			for (auto edge: obj->shape->edgeList) {
				edgeList.push_back({offset+edge[0], offset+edge[1]});
			}

			numEdges += obj->shape->edgeList.size();
			edgeRestLengthSquares.insert(edgeRestLengthSquares.end(), obj->shape->edgeRestLengthSquares.begin(), obj->shape->edgeRestLengthSquares.end());
		}

		initialPositions = positions;
		initialVelocities = velocities;


		//!REMOVE ME
		float maxHeight = positions.col(0).y();
		vector<int> bestidxs = {0};

		for (int i=1; i<numPoints; i++) {
			if (positions.col(i).y() > maxHeight) {
				bestidxs.clear();
				bestidxs.push_back(i);
				maxHeight = positions.col(i).y();
			} else if (positions.col(i).y() == maxHeight) {
				bestidxs.push_back(i);
			}
		}

		for (int idx: bestidxs) {
			isFixedPoint[idx] = true;
		}
		//!
	}
}




// Step functions
void PhysicsEngine::implicitStep() {
	if (physicsObjects.empty()) return;	

	// Make copy of original positions & calculate explicit predicted positions
	Matrix3Xf originalPositions = positions;
	Matrix3Xf predictedPositions = positions + h*velocities;

	// Calculate initial Incremental Potential value and search direction 
	float IP = IPValue(predictedPositions);
	Matrix3Xf searchDirection = getSearchDirection(predictedPositions);

	// Projected Newton Loop
	for (int newtoniter=0; newtoniter<maxiter; newtoniter++) {
		// Line search to guarantees a step size that reduces the systems energy
		float alpha = 1;
		positions = originalPositions + alpha*searchDirection;

		float newIP = IPValue(predictedPositions);

		for (int lineiter=0; lineiter<maxiter; lineiter++) {
			if (newIP < IP) break;

			alpha /= 2;
			positions = originalPositions + alpha*searchDirection;
			newIP = IPValue(predictedPositions);
		}
		
		// Update IP & calculate next search direction
		IP = newIP;
		searchDirection = getSearchDirection(predictedPositions);
		if (searchDirection.cwiseAbs().maxCoeff() < tol) break; // infinity norm early convergence condition
	}

	// Update velocities with final positions
	velocities = (positions - originalPositions) / h;

	// Send new positions to each object and recompute all normals
    updateObjects();
}



// Helper
Matrix3Xf PhysicsEngine::getSearchDirection(Matrix3Xf& xtilde) {
	SparseMatrix<float> hess = IPHessian(xtilde);
	Matrix3Xf grad = IPGradient(xtilde);

	// Gradient sticky DBCs
	for (int vidx=0; vidx<numPoints; vidx++) {
		if (isFixedPoint[vidx]) {
			grad.col(vidx) = Vector3f(0, 0, 0);
		}
	}

	// Hess sticky DBCs
    for (int col=0; col<hess.outerSize(); col++) {
        for (SparseMatrix<float>::InnerIterator it(hess, col); it; ++it) {
			
			int row = it.row();
			if (isFixedPoint[(int) row / 3] || isFixedPoint[(int) col / 3]) {
				it.valueRef() = row == col ? 1 : 0;
			}
        }
    }
 
	// Sparse solver 
	Eigen::SimplicialLDLT<SparseMatrix<float>> solver;
	solver.compute(hess);

    if (solver.info() != Eigen::Success) {
        std::cerr << "Solver failed to compute decompose Hessian!\n";
        return Matrix3Xf::Zero(3, numPoints);
    }

	VectorXf p = solver.solve(-Eigen::Map<VectorXf>(grad.data(), 3*numPoints));
	return Eigen::Map<Matrix3Xf>(p.data(), 3, numPoints);
}
void PhysicsEngine::makePSD(MatrixXf& hess) {

	// Self-adjoint (A = A^T) matrix has real eigenvalues and orthogonal eigenvectors and our local hess
	// is a block of (H, -H; -H, H) which is self-adjoint so we can use the SelfAdjointEigenSolver)
	Eigen::SelfAdjointEigenSolver<MatrixXf> es(hess);
	VectorXf evals = es.eigenvalues();
	MatrixXf evecs = es.eigenvectors();

	// Zero out negative eigenvalues to make PSD
	for (int i=0; i<evals.size(); i++) {
		if (evals(i) < 0) evals(i) = 0;
	}

	// Reconstruct matrix with new eigenvalues
	hess = evecs * evals.asDiagonal() * evecs.transpose();
}
void PhysicsEngine::updateObjects() {
	for (size_t objIdx=0; objIdx<physicsObjects.size(); objIdx++) {
		auto obj = physicsObjects[objIdx];
        int offset = objectOffsets[objIdx];

		auto submatrix = positions.middleCols(offset, obj->numPoints);
		obj->shape->posBuf.assign(submatrix.data(), submatrix.data() + submatrix.size());

		obj->shape->computeNormals();
	}
}
void PhysicsEngine::reset() {
	positions = initialPositions;
	velocities = initialVelocities;

	updateObjects();
}



// Incremental Potential Energy
float PhysicsEngine::IPValue(Matrix3Xf& xtilde) {
	return InertiaValue(xtilde) + h*h*(MassSpringValue() + GravityValue());
}
Matrix3Xf PhysicsEngine::IPGradient(Matrix3Xf& xtilde) {
	return InertiaGradient(xtilde) + h*h*(MassSpringGradient() + GravityGradient());
}
SparseMatrix<float> PhysicsEngine::IPHessian(Matrix3Xf& xtilde) {
	return InertiaHessian(xtilde) + h*h*(MassSpringHessian());
}



// Inertia Energy 
float PhysicsEngine::InertiaValue(Matrix3Xf& xtilde) {
	float sum = 0;
	for (int vidx=0; vidx<numPoints; vidx++) {
		Vector3f diff = positions.col(vidx) - xtilde.col(vidx);
		sum += diff.dot(diff);
	}

	return pointMass * sum / 2;
}
Matrix3Xf PhysicsEngine::InertiaGradient(Matrix3Xf& xtilde) {
	return pointMass * (positions - xtilde);
}
SparseMatrix<float> PhysicsEngine::InertiaHessian(Matrix3Xf& xtilde) {

	// From eigen docs: "The cost of a single purely random insertion into a SparseMatrix is O(nnz), 
	// where nnz is the current number of non-zero coefficients."
	// So it recommends using triplets, which constructs the SparseMatrix in O(n) with n the number of triplets

	int dof = 3*numPoints;
	vector<Triplet<float>> triplets(dof);
	for (int i=0; i<dof; i++) {
		triplets[i] = Triplet<float>(i, i, pointMass);
	}

	SparseMatrix<float> hess(dof, dof);
	hess.setFromSortedTriplets(triplets.begin(), triplets.end());

	return hess;
}



// Mass Spring Energy 
float PhysicsEngine::MassSpringValue() {
	float sum = 0;
	for (int edgeIdx=0; edgeIdx<numEdges; edgeIdx++) {
		auto edge = edgeList[edgeIdx];
		Vector3f diff = positions.col(edge[0]) - positions.col(edge[1]);
		float l2 = edgeRestLengthSquares[edgeIdx];

		sum += l2 * pow(diff.dot(diff) / l2 - 1, 2);
	}
	return sum * springStiffness / 2;
}
Matrix3Xf PhysicsEngine::MassSpringGradient() {
	Matrix3Xf grad = MatrixXf::Zero(3, numPoints);

	for (int edgeIdx=0; edgeIdx<numEdges; edgeIdx++) {
		auto edge = edgeList[edgeIdx];
		Vector3f diff = positions.col(edge[0]) - positions.col(edge[1]);
		float l2 = edgeRestLengthSquares[edgeIdx];

		Vector3f edgeGrad = 2 * springStiffness * (diff.dot(diff) / l2 - 1) * diff;
		grad.col(edge[0]) += edgeGrad;
		grad.col(edge[1]) -= edgeGrad;
	}

	return grad;
}
SparseMatrix<float> PhysicsEngine::MassSpringHessian() {

	int dof = 3*numPoints;
	vector<Triplet<float>> triplets(dof);
	triplets.reserve(9*edgeList.size()); // 2 vertices per edge, each with 3 dofs = 3^2 = 9 second derivatives

	for (int edgeIdx=0; edgeIdx<numEdges; edgeIdx++) {
		auto edge = edgeList[edgeIdx];
		Vector3f diff = positions.col(edge[0]) - positions.col(edge[1]);
		float l2 = edgeRestLengthSquares[edgeIdx];

		// Hessian for the energy of single edge, 3x3 for each DIFFERENCE in the two vertices
		Matrix3f diffHess = 2 * springStiffness / l2 * (2 * diff * diff.transpose() + (diff.dot(diff) - l2) * Matrix3f::Identity());

		// Essemble 6x6 hessian for the 6 DOFs on the two vertices of the edge. diffHess is symmetric, so 
		// this block matrix will also be symmetric so we can use a SelfAdjointEigenSolver to make PSD
		MatrixXf localHess(6, 6);
		localHess.block<3,3>(0,0) = diffHess;
		localHess.block<3,3>(0,3) = -diffHess;
		localHess.block<3,3>(3,0) = -diffHess;
		localHess.block<3,3>(3,3) = diffHess;
		makePSD(localHess);


		for (int blockRow=0; blockRow<=1; blockRow++) {
			for (int blockCol=0; blockCol<=1; blockCol++) {

				int startRow = 3*edge[blockRow];
				int startCol = 3*edge[blockCol];

				for (int row=0; row<3; row++) {
					for (int col=0; col<3 ;col++) {
						float value = localHess(3*blockRow+row, 3*blockCol+col);

						triplets.push_back(Triplet<float>(startRow+row, startCol+col, value));
					}
				}	
			}
		}
	}

	SparseMatrix<float> hess = SparseMatrix<float>(3*numPoints, 3*numPoints);
	hess.setFromTriplets(triplets.begin(), triplets.end());

	return hess;
}



// Gravity Energy 
float PhysicsEngine::GravityValue() {
	float sum = 0;
	for (int vidx=0; vidx<numPoints; vidx++) {
		sum += gravity.dot(positions.col(vidx));
	}

	return -sum * pointMass;
}
Matrix3Xf PhysicsEngine::GravityGradient() {
	Matrix3Xf grad = Matrix3Xf::Zero(3, numPoints);
	for (int vidx=0; vidx<numPoints; vidx++) {
		grad.col(vidx) = -pointMass * gravity;
	}

	return grad;
}

