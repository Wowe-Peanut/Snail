#include "Object.h"
#include "Shape.h"
#include "MatrixStack.h"
#include <algorithm>
#include <iostream>
#include <vector>
#include <memory>

#include "GLSL.h"
#include "Program.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
	
#define EIGEN_DONT_ALIGN_STATICALLY
#include <Eigen/Sparse>
#include <Eigen/Dense>

using namespace std;
using Eigen::Vector3f, Eigen::Matrix3Xf, Eigen::VectorXf, Eigen::MatrixXf, Eigen::Matrix3f;



Matrix3Xf Object::getSearchDirection(Matrix3Xf& xtilde, float h) {
	MatrixXf hess = IPHessian(xtilde, h);
	Matrix3Xf grad = IPGradient(xtilde, h);

	// Apply sticky DBCs
	for (int vidx=0; vidx<numPoints; vidx++) {
		if (isFixedPoint[vidx]) {
			grad.col(vidx) = Vector3f(0, 0, 0);

			// hess.row(vidx).setZero();
			// hess.row(vidx + 1).setZero();
			// hess.row(vidx + 2).setZero();

			// hess.col(vidx).setZero();
			// hess.col(vidx + 1).setZero();
			// hess.col(vidx + 2).setZero();

			// hess.block<3, 3>(vidx, vidx).setIdentity();
		}
	}


	// LDLT is Chomsky Decomposition which is fast at solving Ax = b systems when A is SPD (symmetric positive definite)
	Eigen::LDLT<MatrixXf> solver;
	solver.compute(hess);
	if (solver.info() != Eigen::Success) {
		cerr << "Failed to decompose hessian" << endl;
		return Matrix3Xf::Zero(3, numPoints);
	}

	// Solve for search direction, p = -H^-1 g
	// Map is used to resize the grad matrix to a VectorXf without making a copy
	VectorXf p = solver.solve(-Eigen::Map<VectorXf>(grad.data(), 3*numPoints));

	return Eigen::Map<Matrix3Xf>(p.data(), 3, numPoints);
}

void Object::makePSD(MatrixXf& hess) {

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

// Incremental Potential Energy --------------------------------------------------------------------------------------
float Object::IPValue(Matrix3Xf& xtilde, float h) {
	return InertiaValue(xtilde, h) + h*h*(MassSpringValue(h) + GravityValue(h));
}
Matrix3Xf Object::IPGradient(Matrix3Xf& xtilde, float h) {
	return InertiaGradient(xtilde, h) + h*h*(MassSpringGradient(h) + GravityGradient(h));
}
MatrixXf Object::IPHessian(Matrix3Xf& xtilde, float h) {
	return InertiaHessian(xtilde, h) + h*h*(MassSpringHessian(h));
}






// Inertia Energy -----------------------------------------------------------------------------------------------------
float Object::InertiaValue(Matrix3Xf& xtilde, float h) {
	float sum = 0;
	for (int vidx=0; vidx<numPoints; vidx++) {
		Vector3f diff = positions.col(vidx) - xtilde.col(vidx);
		sum += diff.dot(diff);
	}

	return pointMass * sum / 2;
}
Matrix3Xf Object::InertiaGradient(Matrix3Xf& xtilde, float h) {
	return pointMass * (positions - xtilde);
}
MatrixXf Object::InertiaHessian(Matrix3Xf& xtilde, float h) {
	return pointMass * MatrixXf::Identity(3*numPoints, 3*numPoints);
}






// Mass Spring Energy -------------------------------------------------------------------------------------------------
float Object::MassSpringValue(float h) {
	float sum = 0;
	for (int edgeIdx=0; edgeIdx<numEdges; edgeIdx++) {
		auto edge = shape->edgeList[edgeIdx];
		Vector3f diff = positions.col(edge[0]) - positions.col(edge[1]);
		float l2 = shape->lengthsSquared[edgeIdx];

		sum += l2 * pow(diff.dot(diff) / l2 - 1, 2);
	}
	return sum * springStiffness / 2;
}
Matrix3Xf Object::MassSpringGradient(float h) {
	Matrix3Xf grad = MatrixXf::Zero(3, numPoints);

	for (int edgeIdx=0; edgeIdx<numEdges; edgeIdx++) {
		auto edge = shape->edgeList[edgeIdx];
		Vector3f diff = positions.col(edge[0]) - positions.col(edge[1]);
		float l2 = shape->lengthsSquared[edgeIdx];

		Vector3f edgeGrad = 2 * springStiffness * (diff.dot(diff) / l2 - 1) * diff;
		grad.col(edge[0]) += edgeGrad;
		grad.col(edge[1]) -= edgeGrad;
	}

	return grad;
}
MatrixXf Object::MassSpringHessian(float h) {
	MatrixXf hess = MatrixXf::Zero(3*numPoints, 3*numPoints);

	for (int edgeIdx=0; edgeIdx<numEdges; edgeIdx++) {
		auto edge = shape->edgeList[edgeIdx];
		Vector3f diff = positions.col(edge[0]) - positions.col(edge[1]);
		float l2 = shape->lengthsSquared[edgeIdx];

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




// Gravity Energy -----------------------------------------------------------------------------------------------------
float Object::GravityValue(float h) {
	float sum = 0;
	for (int vidx=0; vidx<numPoints; vidx++) {
		sum += gravity.dot(positions.col(vidx));
	}

	return -sum * pointMass;
}
Matrix3Xf Object::GravityGradient(float h) {
	Matrix3Xf grad = Matrix3Xf::Zero(3, numPoints);
	for (int vidx=0; vidx<numPoints; vidx++) {
		grad.col(vidx) = -pointMass * gravity;
	}

	return grad;
}





// Refactoring will come later, this is just an experiemental phase so just put shit to page
void Object::implicitStepForward(float h, float tol, int maxIter) {

	// Make copy of original positions & calculate explicit predicted positions
	Matrix3Xf originalPositions = positions;
	Matrix3Xf predictedPositions = positions + h*velocities;

	// Calculate initial Incremental Potential value and search direction 
	float IP = IPValue(predictedPositions, h);
	Matrix3Xf searchDirection = getSearchDirection(predictedPositions, h);

	// Projected Newton Loop
	for (int newtoniter=0; newtoniter<maxIter; newtoniter++) {
		// Line search to guarantees a step size that reduces the systems energy
		float alpha = 1;
		positions = originalPositions + alpha*searchDirection;

		float newIP = IPValue(predictedPositions, h);

		for (int lineiter=0; lineiter<maxIter; lineiter++) {
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
}

void Object::symplecticStepForward(float h) {
	Vector3f gravity = Vector3f(0, 0, 0);

	// Currently just Symplectic Euler
	for (int edgeIdx=0; edgeIdx<numEdges; edgeIdx++) {

		// Calculate spring stretch and direction
		auto edge = shape->edgeList[edgeIdx];
		Vector3f diff = positions.col(edge[0]) - positions.col(edge[1]);
		float currentLength = diff.norm();
		float restingLength = sqrt(shape->lengthsSquared[edgeIdx]);

		// Calculate spring force: noramlized direction * stiffness * displacement from rest
		Vector3f springForce = -diff.normalized() * springStiffness * (currentLength - restingLength);

		// Apply spring force to current velocities of both ends of spring
		velocities.col(edge[0]) += h * (gravity + springForce) / pointMass;
		velocities.col(edge[1]) += h * (gravity + -springForce) / pointMass;		
	}

	// Fix points by zeroing out velocity
	for (int i=0; i<numPoints; i++) {
		if (isFixedPoint[i]) {
			velocities.col(i) = Vector3f(0, 0, 0);
		}
	}

	// Increment positions using new velocity values
	positions += h * velocities;
}

Object::Object(shared_ptr<Shape> shape, glm::vec3 trans, glm::vec3 rot, glm::vec3 scale, bool physicsObject): 
shape(shape), translation(trans), rotation(rot), scale(scale), physicsObject(physicsObject), positions(shape->posBuf.data(), 3, shape->posBuf.size()/3) {
	
	// Preprocess physics objects to avoid excessive data copying to and from buffer
	if (physicsObject) {

		// Ensures the shape is using drawElement with an index buffer so we only have
		// to change the position data in a single location (unlike drawArrays)
		assert(shape->procedural);
		numPoints = shape->posBuf.size()/3;
		numEdges = shape->edgeList.size();
		
		// INITIAL CONDITIONS
		velocities = Matrix3Xf::Zero(3, numPoints);
		isFixedPoint = vector<bool>(numPoints, false);

		isFixedPoint[numPoints-1] = true;
		isFixedPoint[numPoints-3] = true;
	}

	
}

void Object::draw(shared_ptr<MatrixStack> MV, shared_ptr<Program> prog) {
	
	// Apply base transformations 
	MV->pushMatrix();
	MV->translate(translation);
	MV->rotate(rotation.x, 1, 0, 0);
	MV->rotate(rotation.y, 0, 1, 0);
	MV->rotate(rotation.z, 0, 0, 1);
	MV->scale(scale);

	// Fix bottom to y=0
	MV->translate(0, -shape->getBaseY(), 0);
	
	// Send properties to GPU
	glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	glUniformMatrix4fv(prog->getUniform("MVIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(MV->topMatrix()))));
	glUniform3fv(prog->getUniform("ka"), 1, glm::value_ptr(ka));
	glUniform3fv(prog->getUniform("kd"), 1, glm::value_ptr(kd));
	glUniform3fv(prog->getUniform("ks"), 1, glm::value_ptr(ks)); 
	glUniform1f(prog->getUniform("s"), s);
	
	// Draw the model
	shape->draw(prog);
	MV->popMatrix();
}