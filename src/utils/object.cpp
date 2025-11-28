
#include "object.h"
#include "mesh.h"
#include "matrix_stack.h"
#include "program.h"
#include "material.h"

#include <iostream>
#include <memory>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

Object::Object(shared_ptr<Mesh> mesh, shared_ptr<Material> material, Transform renderTransform, bool isPhysical): 
mesh(mesh), material(material), renderTransform(renderTransform), isPhysical(isPhysical) {}

Object::Object(shared_ptr<Mesh> mesh, shared_ptr<Material> material, bool isPhysical): 
mesh(mesh), material(material), isPhysical(isPhysical) {
	renderTransform = {glm::vec3(0), glm::vec3(0), glm::vec3(1)};
}

void Object::draw(shared_ptr<MatrixStack> MV, shared_ptr<Program> prog) {
	
	// Apply base transformations 
	MV->pushMatrix();
	MV->translate(renderTransform.translation);
	MV->rotate(renderTransform.rotation.x, 1, 0, 0);
	MV->rotate(renderTransform.rotation.y, 0, 1, 0);
	MV->rotate(renderTransform.rotation.z, 0, 0, 1);
	MV->scale(renderTransform.scale);

	glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	glUniformMatrix4fv(prog->getUniform("MVIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(MV->topMatrix()))));
	material->loadUniforms(prog);
	
	mesh->draw(prog);
	MV->popMatrix();
}