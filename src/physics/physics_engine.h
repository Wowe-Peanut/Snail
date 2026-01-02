#pragma once

#include "object.h"
#include "mesh.h"
#include "json.hpp"

#include <Eigen/Dense>
#include <Eigen/Sparse>




class PhysicsEngine {
    public:
        PhysicsEngine(std::vector<std::shared_ptr<Object>>& objectList, nlohmann::json parameters);
        void reset();
        void implicitStep();

        // Simulation parameters
        double h;
        double tol;
        double springStiffness;
		double pointMass;
        double contactStiffness;
        double contactDistance;
        Eigen::Vector3d gravity;

        // Objects and their ensemble properties
        std::vector<std::shared_ptr<Object>> objects; 
        std::vector<int> offsets; 
        
        int numPoints;
        Eigen::Matrix3Xd positions;
        Eigen::Matrix3Xd velocities;
        std::vector<Edge> edges;
        std::vector<bool> isFixedPoint;
        std::vector<Triangle> surfaceTriangles;

        
        // Used to reset sim
        Eigen::Matrix3Xd initialPositions;
        Eigen::Matrix3Xd initialVelocities;

        // Helper
        void makePSD(Eigen::MatrixXd& hess);
        void updateObjects(); 
        Eigen::Matrix3Xd getSearchDirection(Eigen::Matrix3Xd& xtilde);
        double CCD(Eigen::Matrix3Xd& searchDirection);   

        // Total energy
        double IPValue(Eigen::Matrix3Xd& xtilde);
        Eigen::Matrix3Xd IPGradient(Eigen::Matrix3Xd& xtilde);
        Eigen::SparseMatrix<double> IPHessian(Eigen::Matrix3Xd& xtilde);

        // Inertia
        double InertiaValue(Eigen::Matrix3Xd& xtilde);
        Eigen::Matrix3Xd InertiaGradient(Eigen::Matrix3Xd& xtilde);
        Eigen::SparseMatrix<double> InertiaHessian(Eigen::Matrix3Xd& xtilde);

        // Spring
        double MassSpringValue();
        Eigen::Matrix3Xd MassSpringGradient();
        Eigen::SparseMatrix<double> MassSpringHessian();

        // Gravity
        double GravityValue();
        Eigen::Matrix3Xd GravityGradient();

        // Contact
        double ContactValue();
        Eigen::Matrix3Xd ContactGradient();
        Eigen::SparseMatrix<double> ContactHessian();
};