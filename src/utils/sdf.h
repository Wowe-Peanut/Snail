#pragma once

#include <Eigen/Dense>
#include <memory>

struct SDF {
	virtual float distance(Eigen::Vector3f& point) = 0;
	virtual Eigen::Vector3f distanceGrad(Eigen::Vector3f& point) = 0;
	virtual Eigen::Matrix3f distanceHess(Eigen::Vector3f& point) = 0; 
	virtual float ccd(Eigen::Vector3f& point, Eigen::Vector3f& partialSearchDir) = 0;
};

struct PlaneSDF : SDF {
	Eigen::Vector3f planeNormal;
	Eigen::Vector3f planePoint;
	
	PlaneSDF(Eigen::Vector3f normal, Eigen::Vector3f point);
	float distance(Eigen::Vector3f& point) override;
	Eigen::Vector3f distanceGrad(Eigen::Vector3f& point) override;
	Eigen::Matrix3f distanceHess(Eigen::Vector3f& point) override; 
	float ccd(Eigen::Vector3f& point, Eigen::Vector3f& partialSearchDir) override;
};

// struct MeshSDF : SDF {
// 	std::shared_ptr<Mesh> mesh;

// 	float distance(Eigen::Vector3f point) override;
// 	Eigen::Vector3f distanceGrad(Eigen::Vector3f point) override;
// 	Eigen::Matrix3Xf distanceHess(Eigen::Vector3f point) override; 
// };

