#pragma once

#include <Eigen/Dense>
#include <memory>

struct SDF {
	virtual double distance(Eigen::Vector3d& point) = 0;
	virtual Eigen::Vector3d distanceGrad(Eigen::Vector3d& point) = 0;
	virtual Eigen::Matrix3d distanceHess(Eigen::Vector3d& point) = 0; 
	virtual double ccd(Eigen::Vector3d& point, Eigen::Vector3d& partialSearchDir) = 0;
};

struct PlaneSDF : SDF {
	Eigen::Vector3d planeNormal;
	Eigen::Vector3d planePoint;
	
	PlaneSDF(Eigen::Vector3d normal, Eigen::Vector3d point);
	double distance(Eigen::Vector3d& point) override;
	Eigen::Vector3d distanceGrad(Eigen::Vector3d& point) override;
	Eigen::Matrix3d distanceHess(Eigen::Vector3d& point) override; 
	double ccd(Eigen::Vector3d& point, Eigen::Vector3d& partialSearchDir) override;
};

// struct MeshSDF : SDF {
// 	std::shared_ptr<Mesh> mesh;

// 	double distance(Eigen::Vector3d point) override;
// 	Eigen::Vector3d distanceGrad(Eigen::Vector3d point) override;
// 	Eigen::Matrix3Xf distanceHess(Eigen::Vector3d point) override; 
// };

