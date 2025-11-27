
#include "object.h"
#include "mesh.h"
#include "matrix_stack.h"
#include "program.h"

#include <iostream>
#include <vector>
#include <memory>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#define EIGEN_DONT_ALIGN_STATICALLY
#include <Eigen/Dense>

using namespace std;

Object::Object(string meshpath, glm::vec3 trans, glm::vec3 rot, glm::vec3 scale, bool physicsObject): 
mesh(make_shared<StaticMesh>(meshpath)), translation(trans), rotation(rot), scale(scale), physicsObject(physicsObject) {}

void Object::draw(shared_ptr<MatrixStack> MV, shared_ptr<Program> prog) {
	
	// Apply base transformations 
	MV->pushMatrix();
	MV->translate(translation);
	MV->rotate(rotation.x, 1, 0, 0);
	MV->rotate(rotation.y, 0, 1, 0);
	MV->rotate(rotation.z, 0, 0, 1);
	MV->scale(scale);

	// Send properties to GPU
	glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	glUniformMatrix4fv(prog->getUniform("MVIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(MV->topMatrix()))));
	glUniform3fv(prog->getUniform("ka"), 1, glm::value_ptr(ka));
	glUniform3fv(prog->getUniform("kd"), 1, glm::value_ptr(kd));
	glUniform3fv(prog->getUniform("ks"), 1, glm::value_ptr(ks)); 
	glUniform1f(prog->getUniform("s"), s);
	
	// Draw the model
	mesh->draw(prog);
	MV->popMatrix();
}