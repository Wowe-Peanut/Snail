#pragma once

#include "mesh.h"
#include "matrix_stack.h"
#include "program.h"

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp> 
#define EIGEN_DONT_ALIGN_STATICALLY
#include <Eigen/Dense>

class Object {
	public:	
		
		glm::vec3 renderTranslation;			
		glm::vec3 renderRotation;			
		glm::vec3 renderScale;	

		std::shared_ptr<StaticMesh> mesh;

			

		glm::vec3 ka = glm::vec3(0.2, 0.2, 0.2);
		glm::vec3 kd = glm::vec3(0.8, 0.7, 0.7);
		glm::vec3 ks = glm::vec3(1.0, 0.9, 0.8);
		float s = 200;

		bool physicsObject;	

		Object(std::string meshpath, glm::vec3 trans, glm::vec3 rot, glm::vec3 scale, bool physicsObject);
		void draw(std::shared_ptr<MatrixStack> MV, std::shared_ptr<Program> prog);
};

class 


