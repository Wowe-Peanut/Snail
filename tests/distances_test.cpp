#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

#include "physics/distances.h"
#include <Eigen/Dense>

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>

using namespace std;

const double tol = 0.01;



struct Setup {
	map<string, vector<Eigen::Vector3d>> inputs;
	map<string, Distance> gts;

	Setup() {
		
	}

	~Setup() {
		
	}
};

static Setup globalsetup;


void DCOMP(Distance gt, Distance test) {
	REQUIRE_THAT(test.value, Catch::Matchers::WithinAbs(gt.value, tol));
	REQUIRE(test.grad.isApprox(gt.grad, tol));
	REQUIRE(test.hess.isApprox(gt.hess, tol));
}

TEST_CASE("Point-Point") {
	auto inputs = globalsetup.inputs["PL"];
	DCOMP(globalsetup.gts["PP"], PointPointDist(inputs[0], inputs[1], false));
}

TEST_CASE("Point-Line") {
	auto inputs = globalsetup.inputs["PL"];
	DCOMP(globalsetup.gts["PL"], PointLineDist(inputs[0], inputs[1], inputs[2], false));
}

