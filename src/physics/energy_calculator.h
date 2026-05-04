#pragma once

#include <Eigen/Dense>
#include <Eigen/Sparse>

struct SimParameters;
struct SimState;

struct EnergyCalculator {
	
	SimParameters& params;
	SimState& state;

	EnergyCalculator(SimParameters& params, SimState& state): params(params), state(state) {};

	// Helper 
	void makePSD(Eigen::MatrixXd& mat);

    // Total potential energy
    double potentialValue();
    Eigen::Matrix3Xd potentialGradient();
    Eigen::SparseMatrix<double> potentialHessian();

    // Inertia
    double inertiaValue(Eigen::Matrix3Xd& xtilde);
    Eigen::Matrix3Xd inertiaGradient(Eigen::Matrix3Xd& xtilde);
    Eigen::SparseMatrix<double> inertiaHessian(Eigen::Matrix3Xd& xtilde);

    // Spring
    double massSpringValue();
    Eigen::Matrix3Xd massSpringGradient();
    Eigen::SparseMatrix<double> massSpringHessian();

    // Gravity
    double gravityValue();
    Eigen::Matrix3Xd gravityGradient();

    // Contact
    double contactValue();
    Eigen::Matrix3Xd contactGradient();
    Eigen::SparseMatrix<double> contactHessian();

    // d^2 barrier energy
    double barrier(double d2);
    double barrierD(double d2);
    double barrierD2(double d2);

    // Neo-Hookean Elasticity
    double NHValue();
    Eigen::Matrix3Xd NHGradient();


    
};	