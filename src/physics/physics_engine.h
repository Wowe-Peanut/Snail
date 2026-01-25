#pragma once

#include "time_integrator.h"
#include "object.h"
#include "mesh.h"

#include <Eigen/Dense>
#include <Eigen/Sparse>

struct SimParameters {
    double dt;
    double tolerance;
    double springStiffness;
    double pointMass;
    double contactStiffness;
    double contactDistance;
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
    TimeIntegrator integrator;

    Eigen::Matrix3Xd initialPositions;
    Eigen::Matrix3Xd initialVelocities;

    PhysicsEngine(std::vector<std::shared_ptr<Object>>& objects, SimParameters& params);
    void step(); 
    void reset();
    void updateObjects();
};