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
#include <Eigen/Sparse>
#include <Eigen/Dense>

#include <glm/glm.hpp> 

class Object {
	public:	
		// This is only necessary if we have fixed-size Eigen objects as members (for when you call 'new')
		// EIGEN_MAKE_ALIGNED_OPERATOR_NEW  
		
		std::shared_ptr<Shape> shape;
		glm::vec3 translation;			
		glm::vec3 rotation;			
		glm::vec3 scale;		

		glm::vec3 ka = glm::vec3(0.4, 0.3, 0.3);
		glm::vec3 kd = glm::vec3(0.8, 0.7, 0.7);
		glm::vec3 ks = glm::vec3(1.0, 0.9, 0.8);
		float s = 200;
		
		bool physicsObject;	
		float springStiffness = 10000.0f;
		float pointMass = 500;

		// Eigen::Matrix3Xf is typedef for Eigen::Matrix<float, 3, Eigen::Dynamic>
		Eigen::Map<Eigen::Matrix3Xf> positions;
		Eigen::Matrix3Xf velocities;
		int numPoints;
		int numEdges;

		float IPValue(std::vector<Eigen::Vector3f>& predictedPositions);
		void IPUpdateGradient(std::vector<Eigen::Vector3f>& predictedPositions);
		void IPUpdateHessian(std::vector<Eigen::Vector3f>& predictedPositions);

		void symplecticStepForward(float h);
		void implicitStepForward(float h);
		Object(std::shared_ptr<Shape> shape, glm::vec3 trans, glm::vec3 rot, glm::vec3 scale, bool physicsObject);
		void draw(std::shared_ptr<MatrixStack> MV, std::shared_ptr<Program> prog);
};

