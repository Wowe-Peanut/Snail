#pragma once

#include "mesh.h"

#include <Eigen/Dense>
#include <memory>

struct SDF {
	virtual float distance(Eigen::Vector3f point) = 0;
	virtual Eigen::Vector3f distanceGrad(Eigen::Vector3f point) = 0;
	virtual Eigen::Matrix3Xf distanceHess(Eigen::Vector3f point) = 0; 
};

struct PlaneSDF : SDF {
	Eigen::Vector3f planeNormal;
	Eigen::Vector3f planePoint;
	
	float distance(Eigen::Vector3f point) override;
	Eigen::Vector3f distanceGrad(Eigen::Vector3f point) override;
	Eigen::Matrix3Xf distanceHess(Eigen::Vector3f point) override; 
};

// struct MeshSDF : SDF {
// 	std::shared_ptr<Mesh> mesh;

// 	float distance(Eigen::Vector3f point) override;
// 	Eigen::Vector3f distanceGrad(Eigen::Vector3f point) override;
// 	Eigen::Matrix3Xf distanceHess(Eigen::Vector3f point) override; 
// };

