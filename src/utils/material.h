#pragma once

#include "matrix_stack.h"
#include "program.h"

#include <memory>
#include <glm/glm.hpp> 

class Material {
	public:
		virtual void loadUniforms(std::shared_ptr<Program> prog) = 0;
};

class BPhongMaterial : public Material {
	glm::vec3 ka;
	glm::vec3 kd;
	glm::vec3 ks;
	float s;

	void loadUniforms(std::shared_ptr<Program> prog);
};