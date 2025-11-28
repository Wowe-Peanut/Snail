#pragma once

#include "matrix_stack.h"
#include "program.h"
#include "material.h"

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp> 

class Mesh;

struct Transform {
	glm::vec3 translation;
	glm::vec3 rotation;
	glm::vec3 scale;
};

class Object {
	public:	
		
		std::shared_ptr<Mesh> mesh;
		std::shared_ptr<Material> material;
		Transform renderTransform;
		bool isPhysical;

		// renderTransform is mainly intended for non-physical objects as it is applied by GPU to triangles
		// the default is a identity transform and the json transform is instead applied directly to mesh
		// so as to be reflected in the physics simulation
		Object(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material>, Transform renderTransform, bool isPhysical);
		Object(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material>, bool isPhysical);
		void draw(std::shared_ptr<MatrixStack> MV, std::shared_ptr<Program> prog);
};



