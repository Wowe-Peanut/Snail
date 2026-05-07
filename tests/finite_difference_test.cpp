#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

#include "energy_calculator.h"
#include "physics_engine.h"
#include <Eigen/Dense>

#include <cstdlib>
#include <random>
#include <memory>
#include <iostream>

using namespace std;
using Eigen::MatrixXd, Eigen::Matrix3Xd, Eigen::Vector3d, Eigen::SparseMatrix, Eigen::VectorXd;

double tol = 0.01;
double epsilon = 1e-6;

/**
 * This tests if gradient (and maybe Hessian if implemented) functions match the value function by comparing with finite differences
 * 
 * Based on: https://www.cs.ucr.edu/~craigs/papers/2022-derivatives/course.pdf
 * 
 * Note that Hessian tests will fail if using makePSD
 */

struct Setup {
    shared_ptr<CollisionManager> collisionManager;
    shared_ptr<EnergyCalculator> energyCalculator;

    SimParameters p;
    SimState s;

    int POINTS, EDGES, TRIANGLES, TETS;

	Setup() {

        // Parameters
        p.springStiffness = 500000;
        p.pointMass = 50;
        p.contactStiffness = 200000;
        p.contactDistance = 0.05;
        p.cd2 = (0.05*0.05);
        p.gravity = Vector3d(0, -9.81, 0);
    

        POINTS      = (rand() % 900) + 100;
        EDGES       = (rand() % 900) + 100;
        TRIANGLES   = (rand() % 900) + 100;
        TETS        = (rand() % 900) + 100;



        // Initialize point positions randomly from [-1, 1]^3
        s.numPoints = POINTS;
        s.positions = Eigen::Matrix3Xd::Zero(3, POINTS);
        for (int i=0; i<s.numPoints; i++) {
            s.positions.col(i) = Vector3d::Random();
        }

        // Initialize edges
        for (int i=0; i<EDGES; i++) {
            vector<int> p = getRandomIndices(POINTS, 2);

            Vector3d diff = s.positions.col(p[0]) - s.positions.col(p[1]);
            s.edges.push_back({p[0], p[1], diff.dot(diff)});
        }

        // Initialize triangles
        for (int i=0; i<TRIANGLES; i++) {
            vector<int> p = getRandomIndices(POINTS, 3);
            s.triangles.push_back({p[0], p[1], p[2]});
        }

        // Initialize tets
        for (int i=0; i<TETS; i++) {
            vector<int> p = getRandomIndices(POINTS, 4);
            
            s.tets.push_back({p[0], p[1], p[2], p[3]});
            s.tets.back().init(s.positions.col(p[0]),
                            s.positions.col(p[1]),
                            s.positions.col(p[2]),
                            s.positions.col(p[3]));
        }

        collisionManager = make_shared<CollisionManager>(p, s);
        energyCalculator = make_shared<EnergyCalculator>(p, s);
	} 

    vector<int> getRandomIndices(int sourceSize, int count) {
        vector<int> result;

        while ((int) result.size() < count) {
            int newIndex = rand() % sourceSize;

            if (find(result.begin(), result.end(), newIndex) == result.end()) {
                result.push_back(newIndex);
            }
        }

        return result;
    }

	~Setup() {
		
	}
};

static Setup setup;

void isSmall(double a) {
    REQUIRE(abs(a) < tol);
}

Eigen::Matrix3Xd perturb(double alpha) {
    return Eigen::Matrix3Xd::Random(3, setup.POINTS)*alpha;
}

TEST_CASE("Inertia") {
    SimState& s = setup.s;
    
    // Setup
    Matrix3Xd xtilde = s.positions + perturb(0.01);
    Matrix3Xd dp = perturb(epsilon);
    Matrix3Xd originalPositions = s.positions;
  
    // Original
    s.positions = originalPositions;
    Matrix3Xd               gradient = setup.energyCalculator->inertiaGradient(xtilde);
    SparseMatrix<double>    hessian = setup.energyCalculator->inertiaHessian(xtilde);
  
    // Forward
    s.positions = originalPositions + dp;
    double      forwardValue = setup.energyCalculator->inertiaValue(xtilde);
    Matrix3Xd   forwardGradient = setup.energyCalculator->inertiaGradient(xtilde);
  
    // Backward
    s.positions = originalPositions - dp;
    double      backwardValue = setup.energyCalculator->inertiaValue(xtilde);
    Matrix3Xd   backwardGradient = setup.energyCalculator->inertiaGradient(xtilde);
  
    SECTION("Inertia Gradient") {
        isSmall((forwardValue - backwardValue - 2 * gradient.reshaped().dot(dp.reshaped())) / (2 * epsilon));
    }

    SECTION("Inertia Hessian") {
        isSmall((forwardGradient.reshaped() - backwardGradient.reshaped() - 2 * hessian * dp.reshaped()).norm() / (2 * epsilon));
    }
  
    s.positions = originalPositions;
}

TEST_CASE("Spring") {
    SimState& s = setup.s;
  
    // Setup
    Matrix3Xd dp = perturb(epsilon);
    Matrix3Xd originalPositions = s.positions;
  
    // Original
    s.positions = originalPositions;
    Matrix3Xd               gradient = setup.energyCalculator->massSpringGradient();
    SparseMatrix<double>    hessian = setup.energyCalculator->massSpringHessian();
  
    // Forward
    s.positions = originalPositions + dp;
    double      forwardValue = setup.energyCalculator->massSpringValue();
    Matrix3Xd   forwardGradient = setup.energyCalculator->massSpringGradient();
  
    // Backward
    s.positions = originalPositions - dp;
    double      backwardValue = setup.energyCalculator->massSpringValue();
    Matrix3Xd   backwardGradient = setup.energyCalculator->massSpringGradient();
  
    SECTION("Spring Gradient") {
        isSmall((forwardValue - backwardValue - 2 * gradient.reshaped().dot(dp.reshaped())) / (2 * epsilon));
    } 

    SECTION("Spring Hessian") {
        isSmall((forwardGradient.reshaped() - backwardGradient.reshaped() - 2 * hessian * dp.reshaped()).norm() / (2 * epsilon));
    }
  
    s.positions = originalPositions;
}

TEST_CASE("Gravity") {
    SimState& s = setup.s;
  
    // Setup
    Matrix3Xd dp = perturb(epsilon);
    Matrix3Xd originalPositions = s.positions;
  
    // Original
    s.positions = originalPositions;
    Matrix3Xd gradient = setup.energyCalculator->gravityGradient();
  
    // Forward
    s.positions = originalPositions + dp;
    double forwardValue = setup.energyCalculator->gravityValue();
  
    // Backward
    s.positions = originalPositions - dp;
    double backwardValue = setup.energyCalculator->gravityValue();
  
    SECTION("Gravity") {
        isSmall((forwardValue - backwardValue - 2 * gradient.reshaped().dot(dp.reshaped())) / (2 * epsilon));
    }

    s.positions = originalPositions;
}

TEST_CASE("Contact") {
    SimState& s = setup.s;
  
    // Setup
    setup.collisionManager->broadPhase();
    Matrix3Xd dp = perturb(epsilon);
    Matrix3Xd originalPositions = s.positions;
  
    // Original
    s.positions = originalPositions;
    setup.collisionManager->updateActivePairs(D_VALUE | D_GRAD | D_HESS);
    Matrix3Xd               gradient = setup.energyCalculator->contactGradient();
    SparseMatrix<double>    hessian = setup.energyCalculator->contactHessian();
  
    // Forward
    s.positions = originalPositions + dp;
    setup.collisionManager->updateActivePairs(D_VALUE | D_GRAD | D_HESS);
    double      forwardValue = setup.energyCalculator->contactValue();
    Matrix3Xd   forwardGradient = setup.energyCalculator->contactGradient();
  
    // Backward
    s.positions = originalPositions - dp;
    setup.collisionManager->updateActivePairs(D_VALUE | D_GRAD | D_HESS);
    double      backwardValue = setup.energyCalculator->contactValue();
    Matrix3Xd   backwardGradient = setup.energyCalculator->contactGradient();
  
    SECTION("Contact Gradient") {
        isSmall((forwardValue - backwardValue - 2 * gradient.reshaped().dot(dp.reshaped())) / (2 * epsilon));
    }

    SECTION("Contact Hessian") {
        isSmall((forwardGradient.reshaped() - backwardGradient.reshaped() - 2 * hessian * dp.reshaped()).norm() / (2 * epsilon));
    }
  
    s.positions = originalPositions;
}

TEST_CASE("Barrier") {
    
    auto ec = setup.energyCalculator;

    double center = static_cast<double>(rand()) / static_cast<double>(RAND_MAX) * 2 * setup.p.contactDistance;

    SECTION("Barrier Gradient") {
        isSmall((ec->barrier(center+epsilon) - ec->barrier(center-epsilon))/(2*epsilon) - ec->barrierD(center));
    }

    SECTION("Barrier Hessian") {
        isSmall((ec->barrierD(center+epsilon) - ec->barrierD(center))/epsilon - ec->barrierD2(center));
    }
}

TEST_CASE("SNH") {
    SimState& s = setup.s;
  
    // Setup
    Matrix3Xd dp = perturb(epsilon);
    Matrix3Xd originalPositions = s.positions;
  
    // Original
    s.positions = originalPositions;
    Matrix3Xd gradient = setup.energyCalculator->SNHGradient();
  
    // Forward
    s.positions = originalPositions + dp;
    double forwardValue = setup.energyCalculator->SNHValue();
  
    // Backward
    s.positions = originalPositions - dp;
    double backwardValue = setup.energyCalculator->SNHValue();
  
    SECTION("SNH Gradient") {
        isSmall((forwardValue - backwardValue - 2 * gradient.reshaped().dot(dp.reshaped())) / (2 * epsilon));
    }

    s.positions = originalPositions;
}