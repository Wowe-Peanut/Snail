
#include "parser.h"
#include "integrators.h"
#include "optimizers.h"
#include "object.h"
#include <fstream>
#include <iostream>

using namespace std;
using json = nlohmann::json;
using glm::vec3;

json openjson(string path) {
	ifstream file(path);
	json data;
	
	if (!file.is_open()) {
		cerr << "Failed to open JSON file" << endl;
		exit(1);
	}

	try {
		data = json::parse(file);
	} catch (json::parse_error& ex) {
		cout << "JSON Parse error at byte " << ex.byte << endl;
		exit(1);
	} 

	return data;
}

vec3 toVec3 (json data) {
	return vec3(data[0], data[1], data[2]);
}

Eigen::Vector3d toVector3d(json data) {
	return Eigen::Vector3d(data[0], data[1], data[2]);
}

Transform toTransform(json data) {
	return {toVec3(data["translation"]), toVec3(data["rotation"]), toVec3(data["scale"])};
}

shared_ptr<Material> toMaterial(json data) {
	return make_shared<BPhongMaterial>(toVec3(data["ka"]), toVec3(data["kd"]), toVec3(data["ks"]), data["s"]);
}

vector<shared_ptr<Object>> parseObjects(string resourcePath, string jsonPath) {
	json data = openjson(jsonPath)["objects"];
	
	vector<shared_ptr<Object>> objects;
	for (auto objData: data) {

		// Parse and construct the objects members
		string meshPath = objData["mesh"];
		Transform meshTransform = toTransform(objData["mesh_transform"]);
		vec3 preInitScale = toVec3(objData["pre_init_scale"]);
		shared_ptr<Material> material = toMaterial(objData["material"]);
		vector<int> isDBC = objData["fixed_points"].get<vector<int>>();
		vec3 initialVelocity = toVec3(objData["velocity"]);
		bool isStatic = objData["is_static"];
		
		// Construct mesh & object
		shared_ptr<Mesh> mesh = make_shared<Mesh>(resourcePath + meshPath, isStatic, preInitScale, meshTransform, isDBC, initialVelocity);
		objects.push_back(make_shared<Object>(mesh, material));
	}
	
	return objects;
}

SimParameters parseParameters(string jsonPath) {
	json data = openjson(jsonPath)["parameters"];

	double E = data["E"];
	double v = data["v"];
	string elasticityType = data["elasticity"];

	SimParameters params = {
		data["dt"],
		data["tolerance"],
		data["max_iterations"],

		data["ls_max_iterations"],
		data["ls_contraction"],
		data["ls_lower_bound"],

		data["accd_min_separation"],
		(E/(2*(1+v))),
		(E*v/((1+v)*(1-2*v))),
		data["spring_stiffness"],
		data["point_mass"],
		data["contact_stiffness"],
		data["contact_distance"],
		0,
		toVector3d(data["gravity"]),
		(elasticityType == "springs")
	};

	params.cd2 = params.contactDistance * params.contactDistance;

	return params;
}

shared_ptr<Integrator> parseIntegrator(SimParameters& params, SimState& state, string jsonPath) {
	json data = openjson(jsonPath)["parameters"];

	shared_ptr<ImplicitIntegrator> integrator;
	shared_ptr<Optimizer> optimizer;

	if (data["integrator"] == "backwards_euler")		integrator = make_shared<BackwardsEulerIntegrator>(params, state);
	else {
		cout << "Unrecognized Integrator: " << data["integrator"] << endl;
		exit(1);
	}

	if (data["optimizer"] == "newton") 		optimizer = make_shared<NewtonOptimizer>(params, state);
	else if (data["optimizer"] == "lbfgs") 	optimizer = make_shared<LBFGSOptimizer>(params, state, data["lbfgs_history_size"]);
	else {
		cout << "Unrecognized Optimizer: " << data["optimizer"] << endl;
		exit(1);
	}

	integrator->optimizer = optimizer;
	optimizer->integrator = integrator.get();
	return integrator;
}


