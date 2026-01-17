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


void DCOMP(Distance gt, Distance test) {
	REQUIRE_THAT(test.value, Catch::Matchers::WithinAbs(gt.value, tol));
	REQUIRE(test.grad.isApprox(gt.grad, tol));
	REQUIRE(test.hess.isApprox(gt.hess, tol));
}

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

