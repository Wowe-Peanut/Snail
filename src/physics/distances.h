#pragma once

#include <Eigen/Dense>

//! ALL DISTANCES ARE SQUARED TO AVOID SQRT COMPLEXITY AND INSTABILITY (ADJUSTED BARRIER FUNCTION ACCORDINGLY)

// Returning these objects w/ NRVO allows for the reuse of intermediate values 
struct Distance {
	double value;
	Eigen::Matrix3Xd grad;
	Eigen::MatrixXd hess;
};

// Helper methods
Eigen::Matrix3d asSkewSymmetric(Eigen::Vector3d& v);
Eigen::Matrix3Xd mapGrad(Eigen::Matrix3Xd& grad, int newSize, std::vector<int> idxmap);
Eigen::MatrixXd mapHess(Eigen::MatrixXd& hess, int newSize, std::vector<int> idxmap);
Distance mapDistance(Distance& dist, int newSize, std::vector<int> idxmap, bool valueOnly);

// Helper distance functions
Distance PointPointDist(Eigen::Vector3d& x1, Eigen::Vector3d& x2, bool valueOnly);
Distance PointLineDist(Eigen::Vector3d& x, Eigen::Vector3d& l1, Eigen::Vector3d& l2, bool valueOnly);
Distance PointPlaneDist(Eigen::Vector3d& x, Eigen::Vector3d& p1, Eigen::Vector3d& p2, Eigen::Vector3d& p3, bool valueOnly);
Distance LineLineDist(Eigen::Vector3d& l11, Eigen::Vector3d& l12, Eigen::Vector3d& l21, Eigen::Vector3d& l22, bool valueOnly);
Distance PointEdgeDist(Eigen::Vector3d& x, Eigen::Vector3d& l1, Eigen::Vector3d& l2, bool valueOnly);

// Main distance functions
Distance EdgeEdgeDist(Eigen::Vector3d& e11, Eigen::Vector3d& e12, Eigen::Vector3d& e21, Eigen::Vector3d& e22, bool valueOnly);
Distance PointTriangleDist(Eigen::Vector3d& x, Eigen::Vector3d& t1, Eigen::Vector3d& t2, Eigen::Vector3d& t3, bool valueOnly);
