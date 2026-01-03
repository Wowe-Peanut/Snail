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