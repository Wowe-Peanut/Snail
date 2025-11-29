
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

Object::Object(shared_ptr<Mesh> mesh, shared_ptr<Material> material): mesh(mesh), material(material) {}

void Object::draw(shared_ptr<MatrixStack> MV, shared_ptr<Program> prog) {
	glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	glUniformMatrix4fv(prog->getUniform("MVIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(MV->topMatrix()))));
	material->loadUniforms(prog);	
	mesh->draw(prog);
}