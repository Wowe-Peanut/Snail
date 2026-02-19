#pragma once

#include "integrators.h"
#include "object.h"
#include "mesh.h"

#include <Eigen/Dense>
#include <Eigen/Sparse>

struct Integrator;

struct SimParameters {

    // Optimizer parameters
    double dt;
    double tolerance;
    int maxIter;

    // Line search parameters
    int lsMaxIter;
    double lsContraction;
    double lsLowerBound;

    // ACCD parameters
    double accdMinimumSeparation;

    // Physical constants
    double springStiffness;
    double pointMass;
    double contactStiffness;
    double contactDistance;
    double cd2; 
    Eigen::Vector3d gravity;
};

struct SimState {
    std::vector<std::shared_ptr<Object>> objects;

    int numPoints;
    std::vector<int> offsets; 
    Eigen::Matrix3Xd positions;
    Eigen::Matrix3Xd velocities;

    std::vector<std::shared_ptr<CollisionPair>> activeCollisionPairs;
    std::vector<bool> isDBC;

    std::vector<Edge> edges;
    std::vector<Triangle> triangles;
};

struct PhysicsEngine {
    SimState state;
    SimParameters params;
    std::shared_ptr<Integrator> integrator;

    Eigen::Matrix3Xd initialPositions;
    Eigen::Matrix3Xd initialVelocities;

    PhysicsEngine(std::vector<std::shared_ptr<Object>>& objects, std::string jsonPath);
    void step(); 
    void reset();
    void updateObjects();
};