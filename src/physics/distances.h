#pragma once

#include <Eigen/Dense>

//! ALL DISTANCES ARE SQUARED TO AVOID SQRT COMPLEXITY AND INSTABILITY (BARRIER FUNCTION ADJUSTED ACCORDINGLY)

// PointPoint
double PPval(Eigen::Vector3d& x1, Eigen::Vector3d& x2);
Eigen::Matrix3Xd PPgrad(Eigen::Vector3d& x1, Eigen::Vector3d& x2);
Eigen::MatrixXd PPhess(Eigen::Vector3d& x1, Eigen::Vector3d& x2);

// PointLine
double PLval(Eigen::Vector3d& x, Eigen::Vector3d& l1, Eigen::Vector3d& l2);
Eigen::Matrix3Xd PLgrad(Eigen::Vector3d& x, Eigen::Vector3d& l1, Eigen::Vector3d& l2);
Eigen::MatrixXd PLhess(Eigen::Vector3d& x, Eigen::Vector3d& l1, Eigen::Vector3d& l2);

// PointPlane
double PPlnval(Eigen::Vector3d& x, Eigen::Vector3d& p1, Eigen::Vector3d& p2, Eigen::Vector3d& p3);
Eigen::Matrix3Xd PPlngrad(Eigen::Vector3d& x, Eigen::Vector3d& p1, Eigen::Vector3d& p2, Eigen::Vector3d& p3);
Eigen::MatrixXd PPlnhess(Eigen::Vector3d& x, Eigen::Vector3d& p1, Eigen::Vector3d& p2, Eigen::Vector3d& p3);

// PointEdge
double PEval(Eigen::Vector3d& x, Eigen::Vector3d& e1, Eigen::Vector3d& e2);
Eigen::Matrix3Xd PEgrad(Eigen::Vector3d& x, Eigen::Vector3d& e1, Eigen::Vector3d& e2);
Eigen::MatrixXd PEhess(Eigen::Vector3d& x, Eigen::Vector3d& e1, Eigen::Vector3d& e2);

// PointTriangle
double PTval(Eigen::Vector3d& x, Eigen::Vector3d& t1, Eigen::Vector3d& t2, Eigen::Vector3d& t3);
Eigen::Matrix3Xd PTgrad(Eigen::Vector3d& x, Eigen::Vector3d& t1, Eigen::Vector3d& t2, Eigen::Vector3d& t3);
Eigen::MatrixXd PThess(Eigen::Vector3d& x, Eigen::Vector3d& t1, Eigen::Vector3d& t2, Eigen::Vector3d& t3);

// EdgeEdge
double EEval(Eigen::Vector3d& e11, Eigen::Vector3d& e12, Eigen::Vector3d& e21, Eigen::Vector3d& e22);
Eigen::Matrix3Xd EEgrad(Eigen::Vector3d& e11, Eigen::Vector3d& e12, Eigen::Vector3d& e21, Eigen::Vector3d& e22);
Eigen::MatrixXd EEhess(Eigen::Vector3d& e11, Eigen::Vector3d& e12, Eigen::Vector3d& e21, Eigen::Vector3d& e22);

   


