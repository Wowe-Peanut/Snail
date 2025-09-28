#define _USE_MATH_DEFINES
#include <cmath> 
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include "Camera.h"
#include "MatrixStack.h"

Camera::Camera() :
	aspect(1.0f),
	fovy((float)(45.0*M_PI/180.0)),
	znear(0.1f),
	zfar(1000.0f),
	rotations(0.0, 0.0),
	translations(0.0f, 0.0f, -5.0f),
	rfactor(0.01f),
	tfactor(0.001f),
	sfactor(0.005f)
{
}

Camera::~Camera()
{
}

float Camera::getFOVY() {
	return this->fovy;
}

float Camera::getAspect() {
	return this->aspect;
}

glm::vec3 Camera::getDirV() {
	return glm::vec3(cos(yaw), 0, sin(yaw));
}

glm::vec3 Camera::getRightV() {
	glm::vec3 forward = this->getDirV();
	return glm::vec3(-forward.z, 0, forward.x);
}

void Camera::incFOVY(float delta) {
	this->fovy = bindAngle(fovy+delta, 4, 114);
}

void Camera::mouseClicked(float x, float y, bool shift, bool ctrl, bool alt)
{
	mousePrev.x = x;
	mousePrev.y = y;
	if(shift) {
		state = Camera::TRANSLATE;
	} else if(ctrl) {
		state = Camera::SCALE;
	} else {
		state = Camera::ROTATE;
	}
}

float Camera::bindAngle(float angle, float minDeg, float maxDeg) {
	minDeg *= M_PI/180;
	maxDeg *= M_PI/180;
	return	glm::max(minDeg, glm::min(maxDeg, angle));
}

void Camera::mouseMoved(float x, float y)
{
	glm::vec2 mouseCurr(x, y);
	glm::vec2 dv = mouseCurr - mousePrev;
	switch(state) {
		case Camera::ROTATE:
			yaw += rfactor * dv.x;
			pitch = bindAngle(pitch + rfactor * -dv.y, -60, 60);
			break;
		case Camera::SCALE:
			translations.z *= (1.0f - sfactor * dv.y);
			break;
	}
	mousePrev = mouseCurr;
}

void Camera::applyStaticProjectionMatrix(std::shared_ptr<MatrixStack> P) const
{
	P->multMatrix(glm::perspective((float)(45.0*M_PI/180.0), aspect, znear, zfar));
}

glm::mat4 Camera::getViewMatrix() {
	glm::vec3 forward = this->getDirV();
	forward.y = sin(pitch);

	return glm::lookAt(position, position+forward, glm::vec3(0,1,0));
}

void Camera::applyProjectionMatrix(std::shared_ptr<MatrixStack> P) const
{
	// Modify provided MatrixStack
	P->multMatrix(glm::perspective(fovy, aspect, znear, zfar));
}

void Camera::applyViewMatrix(std::shared_ptr<MatrixStack> MV)
{
	MV->multMatrix(this->getViewMatrix());
}
