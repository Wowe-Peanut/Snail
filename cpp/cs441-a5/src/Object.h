#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cfloat>
#include <random>

#define EIGEN_DONT_ALIGN_STATICALLY
#include <Eigen/Sparse>

#include <glm/glm.hpp> 
class Shape;
class MatrixStack;
class Program;

using namespace std;

class Object {
	public:	
		shared_ptr<Shape> shape;
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
		vector<Eigen::Map<Eigen::Vector3f>> positions;
		vector<Eigen::Vector3f> velocities;
		Eigen::MatrixXf hessian;
		Eigen::VectorXf gradient;

		float IPValue(std::vector<Eigen::Vector3f>& predictedPositions);
		void IPUpdateGradient(std::vector<Eigen::Vector3f>& predictedPositions);
		void IPUpdateHessian(std::vector<Eigen::Vector3f>& predictedPositions);

		void symplecticStepForward(float h);
		void implicitStepForward(float h);
		Object(shared_ptr<Shape> shape, glm::vec3 trans, glm::vec3 rot, glm::vec3 scale, bool physicsObject);
		void draw(shared_ptr<MatrixStack> MV, shared_ptr<Program> prog);
};

