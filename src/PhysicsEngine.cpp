#include "PhysicsEngine.h"
#include <iostream>

#define EIGEN_DONT_ALIGN_STATICALLY
#include <Eigen/Dense>

using json = nlohmann::json;
using namespace std;
using Eigen::Vector3f, Eigen::Matrix3Xf, Eigen::VectorXf, Eigen::MatrixXf, Eigen::Matrix3f;


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
		int bestidx = 0;

		for (int i=1; i<numPoints; i++) {
			if (positions.col(i).y() > maxHeight) {
				bestidx = i;
				maxHeight = positions.col(i).y();
			}
		}

		isFixedPoint[bestidx] = true;
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
	float IP = IPValue(predictedPositions, h);
	Matrix3Xf searchDirection = getSearchDirection(predictedPositions, h);

	// Projected Newton Loop
	for (int newtoniter=0; newtoniter<maxiter; newtoniter++) {
		// Line search to guarantees a step size that reduces the systems energy
		float alpha = 1;
		positions = originalPositions + alpha*searchDirection;

		float newIP = IPValue(predictedPositions, h);

		for (int lineiter=0; lineiter<maxiter; lineiter++) {
			if (newIP < IP) break;

			alpha /= 2;
			positions = originalPositions + alpha*searchDirection;
			newIP = IPValue(predictedPositions, h);
		}
		
		// Update IP & calculate next search direction
		IP = newIP;
		searchDirection = getSearchDirection(predictedPositions, h);
		if (searchDirection.cwiseAbs().maxCoeff() < tol) break; // infinity norm early convergence condition
	}

	// Update velocities with final positions
	velocities = (positions - originalPositions) / h;
    updateObjectPositions();
}



// Helper
Matrix3Xf PhysicsEngine::getSearchDirection(Matrix3Xf& xtilde, float h) {
	MatrixXf hess = IPHessian(xtilde, h);
	Matrix3Xf grad = IPGradient(xtilde, h);

	// Apply sticky DBCs
	for (int vidx=0; vidx<numPoints; vidx++) {
		if (isFixedPoint[vidx]) {
			grad.col(vidx) = Vector3f(0, 0, 0);
		}
	}


	// LDLT is Chomsky Decomposition which is fast at solving Ax = b systems when A is SPD (symmetric positive definite)
	Eigen::LDLT<MatrixXf> solver;
	solver.compute(hess);
	if (solver.info() != Eigen::Success) {
		cout << "Failed to decompose hessian" << endl;
		return Matrix3Xf::Zero(3, numPoints);
	}

	// Solve for search direction, p = -H^-1 g
	// Map is used to resize the grad matrix to a VectorXf without making a copy
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
void PhysicsEngine::updateObjectPositions() {
	for (size_t objIdx=0; objIdx<physicsObjects.size(); objIdx++) {
		auto obj = physicsObjects[objIdx];
        int offset = objectOffsets[objIdx];

		auto submatrix = positions.middleCols(offset, obj->numPoints);
		obj->shape->posBuf.assign(submatrix.data(), submatrix.data() + submatrix.size());
	}
}
void PhysicsEngine::reset() {
	positions = initialPositions;
	velocities = initialVelocities;

	updateObjectPositions();
}



// Incremental Potential Energy
float PhysicsEngine::IPValue(Matrix3Xf& xtilde, float h) {
	return InertiaValue(xtilde, h) + h*h*(MassSpringValue(h) + GravityValue(h));
}
Matrix3Xf PhysicsEngine::IPGradient(Matrix3Xf& xtilde, float h) {
	return InertiaGradient(xtilde, h) + h*h*(MassSpringGradient(h) + GravityGradient(h));
}
MatrixXf PhysicsEngine::IPHessian(Matrix3Xf& xtilde, float h) {
	return InertiaHessian(xtilde, h) + h*h*(MassSpringHessian(h));
}



// Inertia Energy 
float PhysicsEngine::InertiaValue(Matrix3Xf& xtilde, float h) {
	float sum = 0;
	for (int vidx=0; vidx<numPoints; vidx++) {
		Vector3f diff = positions.col(vidx) - xtilde.col(vidx);
		sum += diff.dot(diff);
	}

	return pointMass * sum / 2;
}
Matrix3Xf PhysicsEngine::InertiaGradient(Matrix3Xf& xtilde, float h) {
	return pointMass * (positions - xtilde);
}
MatrixXf PhysicsEngine::InertiaHessian(Matrix3Xf& xtilde, float h) {
	return pointMass * MatrixXf::Identity(3*numPoints, 3*numPoints);
}



// Mass Spring Energy 
float PhysicsEngine::MassSpringValue(float h) {
	float sum = 0;
	for (int edgeIdx=0; edgeIdx<numEdges; edgeIdx++) {
		auto edge = edgeList[edgeIdx];
		Vector3f diff = positions.col(edge[0]) - positions.col(edge[1]);
		float l2 = edgeRestLengthSquares[edgeIdx];

		sum += l2 * pow(diff.dot(diff) / l2 - 1, 2);
	}
	return sum * springStiffness / 2;
}
Matrix3Xf PhysicsEngine::MassSpringGradient(float h) {
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
MatrixXf PhysicsEngine::MassSpringHessian(float h) {
	MatrixXf hess = MatrixXf::Zero(3*numPoints, 3*numPoints);

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

		hess.block<3, 3>(3*edge[0], 3*edge[0]) += localHess.block<3, 3>(0, 0);
		hess.block<3, 3>(3*edge[0], 3*edge[1]) += localHess.block<3, 3>(3, 0); 
		hess.block<3, 3>(3*edge[1], 3*edge[0]) += localHess.block<3, 3>(0, 3); 
		hess.block<3, 3>(3*edge[1], 3*edge[1]) += localHess.block<3, 3>(3, 3);
	}

	return hess;
}



// Gravity Energy 
float PhysicsEngine::GravityValue(float h) {
	float sum = 0;
	for (int vidx=0; vidx<numPoints; vidx++) {
		sum += gravity.dot(positions.col(vidx));
	}

	return -sum * pointMass;
}
Matrix3Xf PhysicsEngine::GravityGradient(float h) {
	Matrix3Xf grad = Matrix3Xf::Zero(3, numPoints);
	for (int vidx=0; vidx<numPoints; vidx++) {
		grad.col(vidx) = -pointMass * gravity;
	}

	return grad;
}





