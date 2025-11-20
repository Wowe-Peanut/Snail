#pragma once

#include "Object.h"
#include "Shape.h"
#include "json.hpp"

#define EIGEN_DONT_ALIGN_STATICALLY
#include <Eigen/Dense>


class PhysicsEngine {
    public:
        PhysicsEngine(std::vector<std::shared_ptr<Object>>& objectList, nlohmann::json parameters);
        void reset();
        void implicitStep();
        void symplecticStep();

        
    private:

        // Simulation parameters
        float h;
        float tol;
        int maxiter;
        float springStiffness;
		float pointMass;
        Eigen::Vector3f gravity;

        std::vector<std::shared_ptr<Object>> physicsObjects; 
        std::vector<int> objectOffsets; // starting index of each physicsObject's point positions in 'positions' & 'velocities'

        // Combined properties of all objects
        int numPoints;
        Eigen::Matrix3Xf positions;
        Eigen::Matrix3Xf velocities;
        std::vector<bool> isFixedPoint;

        Eigen::Matrix3Xf initialPositions;
        Eigen::Matrix3Xf initialVelocities;
        
        int numEdges;
        std::vector<std::vector<int>> edgeList;
        std::vector<float> edgeRestLengthSquares;
        
        // Helper
        void makePSD(Eigen::MatrixXf& hess);
        void updateObjectPositions(); 
        Eigen::Matrix3Xf getSearchDirection(Eigen::Matrix3Xf& xtilde, float h);

        // Total energy
        float IPValue(Eigen::Matrix3Xf& xtilde, float h);
        Eigen::Matrix3Xf IPGradient(Eigen::Matrix3Xf& xtilde, float h);
        Eigen::MatrixXf IPHessian(Eigen::Matrix3Xf& xtilde, float h);

        // Inertia
        float InertiaValue(Eigen::Matrix3Xf& xtilde, float h);
        Eigen::Matrix3Xf InertiaGradient(Eigen::Matrix3Xf& xtilde, float h);
        Eigen::MatrixXf InertiaHessian(Eigen::Matrix3Xf& xtilde, float h);
        
        // Gravity
        float GravityValue(float h);
        Eigen::Matrix3Xf GravityGradient(float h);

        // Spring
        float MassSpringValue(float h);
        Eigen::Matrix3Xf MassSpringGradient(float h);
        Eigen::MatrixXf MassSpringHessian(float h);
};