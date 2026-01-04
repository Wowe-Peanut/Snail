#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

#include "physics/distances.h"
#include <Eigen/Dense>

#include <cstdlib>
#include <iostream>

// Can use mat.isApprox(correct mat, tol) for checking eigen accuracy
const double tol = 0.01;

void DCOMP(double val, double target) {
	REQUIRE_THAT(val, Catch::Matchers::WithinAbs(target, tol));
}

TEST_CASE("PPval") {
    Eigen::Vector3d a(0.0, 0.0, 0.0);
    Eigen::Vector3d b(-1.0, 2.0, 2.0);
	Eigen::Vector3d c(2.0, 1.5, -100.0);

	DCOMP(PPval(a,b), 9.0);
	DCOMP(PPval(b,a), 9.0);
	DCOMP(PPval(a,c), 10006.25);
	DCOMP(PPval(c,a), 10006.25);
	DCOMP(PPval(b,c), 10413.25);
	DCOMP(PPval(c,b), 10413.25);
}

TEST_CASE("PPgrad") {	
	
}
