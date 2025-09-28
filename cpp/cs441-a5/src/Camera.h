#pragma  once
#ifndef CAMERA_H
#define CAMERA_H

#include <memory>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

class MatrixStack;

class Camera
{
public:
	enum {
		ROTATE = 0,
		TRANSLATE,
		SCALE
	};

	glm::vec3 position = glm::vec3(0,0,0);
	float yaw = 0;
	float pitch = 0;
	
	Camera();
	virtual ~Camera();
	void setInitDistance(float z) { translations.z = -std::abs(z); }
	void setAspect(float a) { aspect = a; };
	void setRotationFactor(float f) { rfactor = f; };
	void setTranslationFactor(float f) { tfactor = f; };
	void setScaleFactor(float f) { sfactor = f; };
	void mouseClicked(float x, float y, bool shift, bool ctrl, bool alt);
	void mouseMoved(float x, float y);
	void applyProjectionMatrix(std::shared_ptr<MatrixStack> P) const;
	void applyViewMatrix(std::shared_ptr<MatrixStack> MV);
	glm::vec3 getDirV();
	glm::vec3 getRightV();
	void incFOVY(float delta);
	void applyStaticProjectionMatrix(std::shared_ptr<MatrixStack> P) const;
	float getAspect();
	glm::mat4 getViewMatrix();
	float getFOVY();
	
private:
	float bindAngle(float angle, float min, float max);
	float aspect;
	float fovy;
	float znear;
	float zfar;
	glm::vec2 rotations;
	glm::vec3 translations;
	glm::vec2 mousePrev;
	int state;
	float rfactor;
	float tfactor;
	float sfactor;
};

#endif
