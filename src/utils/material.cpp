
#include "material.h"

#include <glm/glm.hpp> 
#include <glm/gtc/type_ptr.hpp>
#include <memory>

using namespace std;

BPhongMaterial::BPhongMaterial(glm::vec3 ka, glm::vec3 kd, glm::vec3 ks, float s):
ka(ka), kd(kd), ks(ks), s(s) {}

void BPhongMaterial::loadUniforms(shared_ptr<Program> prog) {
	glUniform3fv(prog->getUniform("ka"), 1, glm::value_ptr(ka));
	glUniform3fv(prog->getUniform("kd"), 1, glm::value_ptr(kd));
	glUniform3fv(prog->getUniform("ks"), 1, glm::value_ptr(ks)); 
	glUniform1f(prog->getUniform("s"), s);
}