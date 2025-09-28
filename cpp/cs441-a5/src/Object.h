#pragma once
#ifndef OBJECT_H
#define OBJECT_H
#include <string>
#include <vector>
#include <memory>
#include <cfloat>
#include <random>
using namespace std;

class Object {
	public:	
		shared_ptr<Shape> shape;
		glm::vec3 translation;			
		glm::vec3 rotation;			
		glm::vec3 scale;			
		glm::vec3 originalScale;
		float xy_shear;
		float zy_shear;

		glm::vec3 ke;
		glm::vec3 kd;
		glm::vec3 ks;
		float s;
		
		Object(shared_ptr<Shape> shape, glm::vec3 trans, glm::vec3 rot, glm::vec3 scale, glm::vec3 emissive): 
			   shape(shape), translation(trans), rotation(rot), scale(scale), originalScale(scale),  xy_shear(0), zy_shear(0), ke(emissive) {
			
			kd = minBound(randColor(), 0.3);
			ks = glm::vec3(1.0f, 1.0f, 1.0f);
			s = 10;
		}

		void applyTransform(shared_ptr<MatrixStack> MV) {
			MV->translate(translation);
			MV->rotate(rotation.x, 1, 0, 0);
			MV->rotate(rotation.y, 0, 1, 0);
			MV->rotate(rotation.z, 0, 0, 1);
			MV->scale(scale);
			
			glm::mat4 shearMat(1.0f);
			shearMat[1][0] = xy_shear; 
			shearMat[1][2] = zy_shear;
			MV->multMatrix(shearMat);
		}
	
		void draw(shared_ptr<MatrixStack> MV, shared_ptr<Program> prog) {
			MV->pushMatrix();
			
			applyTransform(MV);
			MV->translate(0, -shape->getBaseY(), 0);
																 
			glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
			glUniformMatrix4fv(prog->getUniform("MVIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(MV->topMatrix()))));
			glUniform3fv(prog->getUniform("ke"), 1, glm::value_ptr(ke));
			glUniform3fv(prog->getUniform("kd"), 1, glm::value_ptr(kd));

			// While doing deffered rendering, we don't generate a specular texture, we can't know which specular
			// values to send to the second pass shaders. Therefore they are now defined as constants in the p2 frag shader

			// glUniform3fv(prog->getUniform("ks"), 1, glm::value_ptr(ks)); 
			// glUniform1f(prog->getUniform("s"), s);

			shape->draw(prog);
			MV->popMatrix();
		}

		void worldRotation(float angle) {
			glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0,1,0));	
			translation = glm::vec3(rotationMatrix * glm::vec4(translation, 1));
		}

	private:
		glm::vec3 randColor() {
			return glm::vec3(randf(), randf(), randf());
		}

		double randf() {
			return ((double) rand()) / RAND_MAX;
		}

		glm::vec3 minBound(glm::vec3 v, float minVal) {
			return glm::vec3(max(minVal, v.x), max(minVal, v.y), max(minVal, v.z));
		}
};


#endif
