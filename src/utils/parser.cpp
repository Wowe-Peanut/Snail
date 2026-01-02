
#include "parser.h"
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
		cerr << "JSON Parse error at byte " << ex.byte << endl;
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

	json data = openjson(jsonPath);
	
	vector<shared_ptr<Object>> objects;
	for (auto objData: data["objects"]) {

		// Parse and construct the objects members
		string meshPath = objData["mesh"];
		Transform meshTransform = toTransform(objData["mesh_transform"]);
		Transform renderTransform = toTransform(objData["render_transform"]);
		shared_ptr<Material> material = toMaterial(objData["material"]);
		vector<int> isDBC = objData["fixed_points"].get<vector<int>>();
		vec3 initialVelocity = toVec3(objData["velocity"]);
		bool isStatic = objData["is_static"];
		
		// Construct mesh & object
		shared_ptr<Mesh> mesh = make_shared<Mesh>(resourcePath + meshPath, isStatic, meshTransform, isDBC, initialVelocity);
		objects.push_back(make_shared<Object>(mesh, material, renderTransform));
	}
	
	return objects;
}

SimParameters parseParameters(string jsonPath) {
	json data = openjson(jsonPath)["parameters"];

	return {
		data["dt"],
		data["tolerance"],
		data["springStiffness"],
		data["pointMass"],
		data["contactStiffness"],
		data["contactDistance"],
		toVector3d(data["gravity"])
	};
}



