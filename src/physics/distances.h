#pragma once

#include <Eigen/Dense>

//! ALL DISTANCES ARE SQUARED TO AVOID SQRT COMPLEXITY AND INSTABILITY (ADJUSTED BARRIER FUNCTION ACCORDINGLY)

// Returning these objects w/ NRVO allows for the reuse of intermediate values 
struct Distance {
	double value;
	Eigen::Matrix3Xd grad;
	Eigen::MatrixXd hess;
};

const int D_VALUE = 0;
const int D_GRAD  = 1;
const int D_HESS  = 2;

// Helper methods
Eigen::Matrix3d asSkewSymmetric(const Eigen::Vector3d& v);
Eigen::Matrix3Xd mapGrad(Eigen::Matrix3Xd& grad, int newSize, std::vector<int> idxmap);
Eigen::MatrixXd mapHess(Eigen::MatrixXd& hess, int newSize, std::vector<int> idxmap);
Distance mapDistance(Distance& dist, int newSize, std::vector<int> idxmap, int flags);

// Helper distance functions
Distance PointPointDist(const Eigen::Vector3d& x1, const Eigen::Vector3d& x2, int flags);
Distance PointLineDist(const Eigen::Vector3d& x, const Eigen::Vector3d& l1, const Eigen::Vector3d& l2, int flags);
Distance PointPlaneDist(const Eigen::Vector3d& x, const Eigen::Vector3d& p1, const Eigen::Vector3d& p2, const Eigen::Vector3d& p3, int flags);
Distance LineLineDist(const Eigen::Vector3d& l11, const Eigen::Vector3d& l12, const Eigen::Vector3d& l21, const Eigen::Vector3d& l22, int flags);
Distance PointEdgeDist(const Eigen::Vector3d& x, const Eigen::Vector3d& l1, const Eigen::Vector3d& l2, int flags);

// Main distance functions
Distance EdgeEdgeDist(const Eigen::Vector3d& e11, const Eigen::Vector3d& e12, const Eigen::Vector3d& e21, const Eigen::Vector3d& e22, int flags);
Distance PointTriangleDist(const Eigen::Vector3d& x, const Eigen::Vector3d& t1, const Eigen::Vector3d& t2, const Eigen::Vector3d& t3, int flags);