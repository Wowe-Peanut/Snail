#pragma once

#include "object.h"
#include "shape.h"
#include "json.hpp"

#include <Eigen/Dense>
#include <Eigen/Sparse>


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
        
        // Combined properties of all objects & their offsets in the cumulative vectors
        int numPoints;
        std::vector<int> objectOffsets; 
        Eigen::Matrix3Xf positions;
        Eigen::Matrix3Xf velocities;

        // Used to reset sim
        Eigen::Matrix3Xf initialPositions;
        Eigen::Matrix3Xf initialVelocities;
        
        // Spring Edges
        int numEdges;
        std::vector<std::vector<int>> edgeList;
        std::vector<float> edgeRestLengthSquares;
        
        // Constraints
        std::vector<bool> isFixedPoint;
        std::vector<int> obstacleContactPoints;



        // Helper
        void makePSD(Eigen::MatrixXf& hess);
        void updateObjects(); 
        Eigen::Matrix3Xf getSearchDirection(Eigen::Matrix3Xf& xtilde);

        // Total energy
        float IPValue(Eigen::Matrix3Xf& xtilde);
        Eigen::Matrix3Xf IPGradient(Eigen::Matrix3Xf& xtilde);
        Eigen::SparseMatrix<float> IPHessian(Eigen::Matrix3Xf& xtilde);

        // Inertia
        float InertiaValue(Eigen::Matrix3Xf& xtilde);
        Eigen::Matrix3Xf InertiaGradient(Eigen::Matrix3Xf& xtilde);
        Eigen::SparseMatrix<float> InertiaHessian(Eigen::Matrix3Xf& xtilde);

        // Spring
        float MassSpringValue();
        Eigen::Matrix3Xf MassSpringGradient();
        Eigen::SparseMatrix<float> MassSpringHessian();

        // Gravity
        float GravityValue();
        Eigen::Matrix3Xf GravityGradient();
};