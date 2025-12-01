
#define GLM_FORCE_RADIANS

#include "render_engine.h"
#include "physics_engine.h"
#include "mesh.h"
#include "material.h"
#include "object.h"
#include "json.hpp"

#include <fstream>
#include <iostream>
#include <glm/glm.hpp>

using json = nlohmann::json;
using vec3 = glm::vec3;
using namespace std;


vec3 jsontovec3 (json jsonlist) {
	return vec3(jsonlist[0], jsonlist[1], jsonlist[2]);
}

json openjson(string path) {
	ifstream f(path);
	json data;
	
	if (!f.is_open()) {
		cerr << "Failed to open JSON file" << endl;
		exit(1);
	}

	try {
		data = json::parse(f);
	} catch (json::parse_error& ex) {
		cerr << "JSON Parse error at byte " << ex.byte << endl;
		exit(1);
	} 

	return data;
}

vector<shared_ptr<Object>> parseObjects(string resourcePath, json objectListJson) {

	vector<shared_ptr<Object>> objects;


	for (auto objjson: objectListJson) {

		// Read JSON sections
		string 			meshpath 		= objjson["mesh"];
		json 			meshtfjson 		= objjson["mesh_transform"];
		json 			rendertfjson 	= objjson["render_transform"];
		json 			matjson 		= objjson["material"];
		vector<int> 	fixedPoints 	= objjson["fixed_points"].get<vector<int>>();
		vec3 			velocity 		= jsontovec3(objjson["velocity"]);
		bool 			isStatic 		= objjson["is_static"];
		
		// Construct mesh
		Transform meshTransform = {jsontovec3(meshtfjson["translation"]), jsontovec3(meshtfjson["rotation"]), jsontovec3(meshtfjson["scale"])};
		shared_ptr<Mesh> mesh = make_shared<Mesh>(resourcePath + meshpath, isStatic, meshTransform, fixedPoints, velocity);
		
		// Construct object
		Transform renderTransform = {jsontovec3(rendertfjson["translation"]), jsontovec3(rendertfjson["rotation"]), jsontovec3(rendertfjson["scale"])};
		shared_ptr<Material> mat = make_shared<BPhongMaterial>(jsontovec3(matjson["ka"]), jsontovec3(matjson["kd"]), jsontovec3(matjson["ks"]), matjson["s"]);
		objects.push_back(make_shared<Object>(mesh, mat, renderTransform));
	}
	
	return objects;
}

void simulate(string resourcePath, string jsonPath) {
	
	json data = openjson(resourcePath + jsonPath);
	vector<shared_ptr<Object>> objects = parseObjects(resourcePath, data["objects"]);

	RenderEngine renderer(objects, resourcePath);
	PhysicsEngine engine(objects, data["parameters"]);
	
	while (!glfwWindowShouldClose(renderer.window)) {
		if (renderer.PAUSED) {
			if (renderer.STEP) {
				engine.implicitStep();
				renderer.STEP = false;
			}
		} else {
			engine.implicitStep();
		}

		if (renderer.RESET) {
			engine.reset();
			renderer.RESET = false;
		}

		renderer.render();
		glfwSwapBuffers(renderer.window);
		glfwPollEvents();
	}

	renderer.bphongProg->unbind();
	glfwDestroyWindow(renderer.window);
	glfwTerminate();
}

int main(int argc, char **argv) {
	if (argc < 3) {
		cerr << "BAD USAGE - A5 RESOURCEDIR JSON" << endl;
		return 1;
	}

	string resourcePath = argv[1] + string("/");
	string jsonPath = argv[2];

	simulate(resourcePath, jsonPath);
	return 0;
}