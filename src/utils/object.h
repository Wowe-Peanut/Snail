#pragma once

#include "matrix_stack.h"
#include "program.h"
#include "material.h"
#include "mesh.h"

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp> 

struct Mesh;

struct Object {
		
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;

	Object(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material>);
	void draw(std::shared_ptr<MatrixStack> MV, std::shared_ptr<Program> prog);
};



