#pragma once
#ifndef OBJECT_H
#define OBJECT_H
#include <string>
#include <vector>
#include <memory>
#include <cfloat>
#include <random>

#define EIGEN_DONT_ALIGN_STATICALLY
#include <Eigen/Dense>

using namespace std;

class Object {
	public:	
		shared_ptr<Shape> shape;
		vector<Eigen::Map<Eigen::Vector3f>> vertexPositions;
		glm::vec3 translation;			
		glm::vec3 rotation;			
		glm::vec3 scale;		
		bool physicsObject;	

		glm::vec3 ka = glm::vec3(0.4, 0.3, 0.3);
		glm::vec3 kd = glm::vec3(0.8, 0.7, 0.7);
		glm::vec3 ks = glm::vec3(1.0, 0.9, 0.8);
		float s = 200;
		
		void stepForward() {

		}

		Object(shared_ptr<Shape> shape, glm::vec3 trans, glm::vec3 rot, glm::vec3 scale, bool physicsObject = false): 
		shape(shape), translation(trans), rotation(rot), scale(scale), physicsObject(physicsObject) {
			

			// Preprocess physics objects to avoid excessive data copying to and from buffer
			if (physicsObject) {

				// Ensures the shape is using drawElement with an index buffer so we only have
				// to change the position data in a single location (unlike drawArrays)
				assert(shape->procedural);

				// Map each Eigen::vec3f to its respective location in the buffer
				vertexPositions = vector<Eigen::Map<Eigen::Vector3f>>();
				for (size_t vidx=0; vidx<vertexPositions.size(); vidx++) {
					vertexPositions.emplace_back(&(shape->posBuf[3*vidx]));
				}
			}

			
		}
		
		void draw(shared_ptr<MatrixStack> MV, shared_ptr<Program> prog) {
			
			// Apply base transformations 
			MV->pushMatrix();
			MV->translate(translation);
			MV->rotate(rotation.x, 1, 0, 0);
			MV->rotate(rotation.y, 0, 1, 0);
			MV->rotate(rotation.z, 0, 0, 1);
			MV->scale(scale);

			// Fix bottom to y=0
			MV->translate(0, -shape->getBaseY(), 0);
			
			// Send properties to GPU
			glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
			glUniformMatrix4fv(prog->getUniform("MVIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(MV->topMatrix()))));
			glUniform3fv(prog->getUniform("ka"), 1, glm::value_ptr(ka));
			glUniform3fv(prog->getUniform("kd"), 1, glm::value_ptr(kd));
			glUniform3fv(prog->getUniform("ks"), 1, glm::value_ptr(ks)); 
			glUniform1f(prog->getUniform("s"), s);
			
			// Draw the model
			shape->draw(prog);
			MV->popMatrix();
		}
};


#endif
