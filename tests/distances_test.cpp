#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

#include "physics/distances.h"
#include <Eigen/Dense>

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>

using namespace std;
using Eigen::MatrixXd, Eigen::Matrix3Xd, Eigen::Vector3d;

const double tol = 0.01;



struct Setup {
	map<string, vector<Eigen::Vector3d>> inputs;
	map<string, Distance> gts;

	Setup() {
		ifstream file("/home/rpomullan/proj/sims/tests/tests.txt"); // stop doxxing me man...

		if (file.is_open()) {

			int cases;
			file >> cases;
			for (int i=0; i<cases; i++) {

				// Name and number of points
				string name;
				int dim;
				file >> name >> dim;

	
				// Input vectors for each point
				vector<Eigen::Vector3d> input_list;
				for (int p=0; p<dim; p++) {
					double x, y, z;
					file >> x >> y >> z;
					input_list.emplace_back(x, y, z);
				}
	
				// Read in ground truth values
				Distance gt;
				file >> gt.value;
	
				gt.grad = Eigen::Matrix3Xd(3, dim);
				for (int c=0; c<dim; c++) {
					for (int r=0; r<3; r++) {
						file >> gt.grad(r, c);
					}
				}
	
				gt.hess = Eigen::MatrixXd(dim*3, dim*3);
				for (int r=0; r<dim*3; r++) {
					for (int c=0; c<dim*3; c++) {
						file >> gt.hess(r, c);
					}
				}
				
				inputs.insert({name, input_list});
				gts.insert({name, gt});
			}
			
		} else {
			cout << "### Could not find test cases ###" << endl;
		}
	} 

	~Setup() {
		
	}
};

static Setup globalsetup;


// Helper method that checks if two Distance structs are approximately the same
void DCOMP(Distance gt, Distance test) {
	REQUIRE_THAT(test.value, Catch::Matchers::WithinAbs(gt.value, tol));
	REQUIRE(test.grad.isApprox(gt.grad, tol));
	REQUIRE(test.hess.isApprox(gt.hess, tol));
}

// Base case distance functions
TEST_CASE("PointPoint") {
	auto inputs = globalsetup.inputs["PointPoint"];
	DCOMP(globalsetup.gts["PointPoint"], PointPointDist(inputs[0], inputs[1], false));
}

TEST_CASE("PointLine") {
	auto inputs = globalsetup.inputs["PointLine"];
	DCOMP(globalsetup.gts["PointLine"], PointLineDist(inputs[0], inputs[1], inputs[2], false));
}

TEST_CASE("PointPlane") {
	auto inputs = globalsetup.inputs["PointPlane"];
	DCOMP(globalsetup.gts["PointPlane"], PointPlaneDist(inputs[0], inputs[1], inputs[2], inputs[3], false));
}

TEST_CASE("LineLine") {
	auto inputs = globalsetup.inputs["LineLine"];
	DCOMP(globalsetup.gts["LineLine"], LineLineDist(inputs[0], inputs[1], inputs[2], inputs[3], false));
}


// Helper method test cases
TEST_CASE("Map Gradient") {

	Vector3d ones = {1, 1, 1};

	// Mapping 3x2 to 3x4 ----------------------------

	// Ground truths
	Matrix3Xd test01(3, 4);
	test01 << 1*ones, 2*ones, 0*ones, 0*ones;

	Matrix3Xd test12(3, 4);
	test12 << 0*ones, 1*ones, 2*ones, 0*ones;

	Matrix3Xd test31(3, 4);
	test31 << 0*ones, 2*ones, 0*ones, 1*ones;

	// Testing map func against ground truths
	Matrix3Xd old3x2(3, 2);
	old3x2 << 1*ones, 2*ones;

	vector<int> idxmap = {0, 1};
	Matrix3Xd new01 = mapGrad(old3x2, 4, idxmap);
	REQUIRE(new01.isApprox(test01, tol));

	idxmap = {1, 2};
	Matrix3Xd new12 = mapGrad(old3x2, 4, idxmap);
	REQUIRE(new12.isApprox(test12, tol));

	idxmap = {3, 1};
	Matrix3Xd new31 = mapGrad(old3x2, 4, idxmap);
	REQUIRE(new31.isApprox(test31, tol));


	// Mapping 3x3 to 3x4 ----------------------------

	// Ground truths
	Matrix3Xd test012(3, 4);
	test012 << 1*ones, 2*ones, 3*ones, 0*ones;

	Matrix3Xd test312(3, 4);
	test312 << 0*ones, 2*ones, 3*ones, 1*ones;

	Matrix3Xd test201(3, 4);
	test201 << 2*ones, 3*ones, 1*ones, 0*ones;

	// Testing map against ground truths
	Matrix3Xd old3x3(3, 3);
	old3x3 << 1*ones, 2*ones, 3*ones;

	idxmap = {0, 1, 2};
	Matrix3Xd new012 = mapGrad(old3x3, 4, idxmap);
	REQUIRE(new012.isApprox(test012, tol));

	idxmap = {3, 1, 2};
	Matrix3Xd new312 = mapGrad(old3x3, 4, idxmap);
	REQUIRE(new312.isApprox(test312, tol));

	idxmap = {2, 0, 1};
	Matrix3Xd new201 = mapGrad(old3x3, 4, idxmap);
	REQUIRE(new201.isApprox(test201, tol));
}

TEST_CASE("Map Hessian") {
	Eigen::Matrix3d ones = Eigen::Matrix3d::Ones();

	MatrixXd test01(12, 12);
	test01 << 	1*ones, 2*ones, 0*ones, 0*ones,
				3*ones, 4*ones, 0*ones, 0*ones,
				0*ones, 0*ones, 0*ones, 0*ones,
				0*ones, 0*ones, 0*ones, 0*ones;

	MatrixXd test31(12, 12);
	test31 << 	0*ones, 0*ones, 0*ones, 0*ones,
				0*ones, 4*ones, 0*ones, 3*ones,
				0*ones, 0*ones, 0*ones, 0*ones,
				0*ones, 2*ones, 0*ones, 1*ones;

	MatrixXd old6x6(6, 6);
	old6x6 << 	1*ones, 2*ones,
				3*ones, 4*ones;

	vector<int> idxmap = {0, 1};
	MatrixXd new01 = mapHess(old6x6, 4, idxmap);
	REQUIRE(new01.isApprox(test01, tol));

	idxmap = {3, 1};
	MatrixXd new31 = mapHess(old6x6, 4, idxmap);
	REQUIRE(new31.isApprox(test31, tol));
}

