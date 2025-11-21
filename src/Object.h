#pragma once

#include "Shape.h"
#include "MatrixStack.h"
#include "Program.h"

#include <string>
#include <vector>
#include <memory>
#include <cfloat>
#include <random>

#define EIGEN_DONT_ALIGN_STATICALLY
#include <Eigen/Dense>

#include <glm/glm.hpp> 

class Object {
	public:	
		
		std::shared_ptr<Shape> shape;
		glm::vec3 translation;			
		glm::vec3 rotation;			
		glm::vec3 scale;		

		glm::vec3 ka = glm::vec3(0.2, 0.2, 0.2);
		glm::vec3 kd = glm::vec3(0.8, 0.7, 0.7);
		glm::vec3 ks = glm::vec3(1.0, 0.9, 0.8);
		float s = 200;		

		bool physicsObject;	
		int numPoints;
		int numEdges;

		Object(std::string meshpath, glm::vec3 trans, glm::vec3 rot, glm::vec3 scale, bool physicsObject);
		void draw(std::shared_ptr<MatrixStack> MV, std::shared_ptr<Program> prog);
};

