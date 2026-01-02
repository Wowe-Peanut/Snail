#pragma once

#include "matrix_stack.h"
#include "program.h"

#include <memory>
#include <glm/glm.hpp> 

struct Material {
	virtual void loadUniforms(std::shared_ptr<Program> prog) = 0;
};

struct BPhongMaterial : Material {
	glm::vec3 ka;
	glm::vec3 kd;
	glm::vec3 ks;
	float s;

	BPhongMaterial(glm::vec3 ka, glm::vec3 kd, glm::vec3 ks, float s);
	void loadUniforms(std::shared_ptr<Program> prog);
};