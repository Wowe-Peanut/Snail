#pragma once

#include <Eigen/Dense>

//! ALL DISTANCES ARE SQUARED TO AVOID SQRT COMPLEXITY AND INSTABILITY (BARRIER FUNCTION ADJUSTED ACCORDINGLY)

// Returning these objects w/ NRVO allows for the reuse of intermediate values 
struct Distance {
	double value;
	Eigen::Matrix3Xd grad;
	Eigen::MatrixXd hess;
};

// Helper methods
Eigen::Matrix3d asSkewSymmetric(Eigen::Vector3d& v);

// Calculates the full value/grad/hess
Distance PointPointDist(Eigen::Vector3d& x1, Eigen::Vector3d& x2, bool valueOnly);
Distance PointLineDist(Eigen::Vector3d& x, Eigen::Vector3d& l1, Eigen::Vector3d& l2, bool valueOnly);

// Only calculates value/grad --> hessian = zero (please forgive me!)
Distance PointPlaneDist(Eigen::Vector3d& x, Eigen::Vector3d& p1, Eigen::Vector3d& p2, Eigen::Vector3d& p3, bool valueOnly);
Distance LineLineDist(Eigen::Vector3d& l11, Eigen::Vector3d& l12, Eigen::Vector3d& l21, Eigen::Vector3d& l22, bool valueOnly);

// Base cases
Distance EdgeEdgeDist(Eigen::Vector3d& e11, Eigen::Vector3d& e12, Eigen::Vector3d& e21, Eigen::Vector3d& e22, bool valueOnly);
Distance PointTriangleDist(Eigen::Vector3d& x, Eigen::Vector3d& t1, Eigen::Vector3d& t2, Eigen::Vector3d& t3, bool valueOnly);
