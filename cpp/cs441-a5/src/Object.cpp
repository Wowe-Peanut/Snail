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
#include <Eigen/Dense>

using namespace std;

void Object::stepForward(float h) {
	// Currently just Symplectic Euler
	for (int edgeIdx=0; edgeIdx<shape->edgeList.size(); edgeIdx++) {

		// Calculate spring stretch and direction
		auto edge = shape->edgeList[edgeIdx];
		Eigen::Vector3f diff = vertexPositions[edge[0]] - vertexPositions[edge[1]];
		float currentLength = diff.norm();
		float restingLength = shape->lengths[edgeIdx];

		// Calculate spring force: noramlized direction * stiffness * displacement from rest

		Eigen::Vector3f springForce = -diff.normalized() * springStiffness * (currentLength - restingLength);

		// Apply spring force to current velocities of both ends of spring
		vertexVelocities[edge[0]] += h * springForce / pointMass;
		vertexVelocities[edge[1]] += h * -springForce / pointMass;				
	}

	// Increment positions using new velocity values
	for (int vidx=0; vidx<vertexPositions.size(); vidx++) {
		vertexPositions[vidx] += h * vertexVelocities[vidx];
	}

}

Object::Object(shared_ptr<Shape> shape, glm::vec3 trans, glm::vec3 rot, glm::vec3 scale, bool physicsObject): 
shape(shape), translation(trans), rotation(rot), scale(scale), physicsObject(physicsObject) {
	
	// Preprocess physics objects to avoid excessive data copying to and from buffer
	if (physicsObject) {

		// Ensures the shape is using drawElement with an index buffer so we only have
		// to change the position data in a single location (unlike drawArrays)
		assert(shape->procedural);

		// Map each Eigen::vec3f to its respective location in the buffer
		vertexPositions = vector<Eigen::Map<Eigen::Vector3f>>();
		for (size_t vidx=0; vidx<shape->posBuf.size()/3; vidx++) {
			vertexPositions.emplace_back(&(shape->posBuf[3*vidx]));
		}


		// Initialize gradient, hessian, and velocity list
		hessian = Eigen::MatrixXf::Zero(vertexPositions.size(), vertexPositions.size());
		gradient = Eigen::VectorXf::Zero(vertexPositions.size());
		vertexVelocities = vector<Eigen::Vector3f>(vertexPositions.size(), Eigen::Vector3f::Zero(3));

		// Initial stretch
		vertexPositions[0] += Eigen::Vector3f(-0.2, -0.2, -0.2);
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