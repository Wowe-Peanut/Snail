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

using namespace std;
using Eigen::Vector3f, Eigen::MatrixXf, Eigen::VectorXf;

float Object::IPValue(vector<Vector3f>& predictedPositions) {
	return 0;
}

void Object::IPUpdateGradient(vector<Vector3f>& predictedPositions) {
	
}

void Object::IPUpdateHessian(vector<Vector3f>& predictedPositions) {

}


// Refactoring will come later, this is just an experiemental phase so just put shit to page
void Object::implicitStepForward(float h) {

	// define tolerance
	float tol = 0.01;

	// Calculate x_tilde, the implicit predictive positions
	// Make copy of current position, x_n
	vector<Vector3f> predictedPositions;
	vector<Vector3f> originalPositions;
	for (int vidx=0; vidx<positions.size(); vidx++) {
		predictedPositions.emplace_back(positions[vidx]);
		predictedPositions.push_back(positions[vidx] + h*velocities[vidx]);
	}

	// Calculate inital incremental potential E(x)


	// Calculate search direction

	// while inf norm of search direction (max abs component) / timeDelta < tol:
	//		line search for stepsize alpha
	// 		update positions with search_direction * alpha
	// 		update current E(x) value
	//		calculate new search direction


	// Using new positions and old positions, calculate new velocities (x_new - x_old) / h

}

void Object::symplecticStepForward(float h) {
	Vector3f gravity = Vector3f(0, -9.81, 0);

	// Currently just Symplectic Euler
	for (int edgeIdx=0; edgeIdx<shape->edgeList.size(); edgeIdx++) {

		// Calculate spring stretch and direction
		auto edge = shape->edgeList[edgeIdx];
		Vector3f diff = positions[edge[0]] - positions[edge[1]];
		float currentLength = diff.norm();
		float restingLength = shape->lengths[edgeIdx];

		// Calculate spring force: noramlized direction * stiffness * displacement from rest

		Vector3f springForce = -diff.normalized() * springStiffness * (currentLength - restingLength);

		// Apply spring force to current velocities of both ends of spring
		velocities[edge[0]] += h * (gravity + springForce) / pointMass;
		velocities[edge[1]] += h * (gravity + -springForce) / pointMass;		
	}

	// Fix points by zeroing out velocity
	velocities[velocities.size()-1] = Vector3f(0, 0, 0);
	velocities[velocities.size()-3] = Vector3f(0, 0, 0);


	// Increment positions using new velocity values
	for (int vidx=0; vidx<positions.size(); vidx++) {
		positions[vidx] += h * velocities[vidx];
	}

}

Object::Object(shared_ptr<Shape> shape, glm::vec3 trans, glm::vec3 rot, glm::vec3 scale, bool physicsObject): 
shape(shape), translation(trans), rotation(rot), scale(scale), physicsObject(physicsObject) {
	
	// Preprocess physics objects to avoid excessive data copying to and from buffer
	if (physicsObject) {

		// Ensures the shape is using drawElement with an index buffer so we only have
		// to change the position data in a single location (unlike drawArrays)
		assert(shape->procedural);

		// Map each Vector3f to its respective location in the buffer
		positions = vector<Eigen::Map<Vector3f>>();
		for (size_t vidx=0; vidx<shape->posBuf.size()/3; vidx++) {
			positions.emplace_back(&(shape->posBuf[3*vidx]));
		}


		// Initialize gradient, hessian, and velocity list
		hessian = MatrixXf::Zero(positions.size(), positions.size());
		gradient = VectorXf::Zero(positions.size());
		velocities = vector<Vector3f>(positions.size(), Vector3f::Zero(3));
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