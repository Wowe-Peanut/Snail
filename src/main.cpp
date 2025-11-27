
#include "render_engine.h"
#include "physics_engine.h"
#include "object.h"
#include "json.hpp"

#include <fstream>
#include <iostream>

#define GLM_FORCE_RADIANS
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

	for (auto obj: objectListJson) {
		string meshpath = obj["mesh"];
		json transform = obj["transformation"];
		vec3 translation = jsontovec3(transform["translation"]);
		vec3 scale = jsontovec3(transform["scale"]);
		vec3 rotation = jsontovec3(transform["rotation"]);
		bool isPhysicsObject = obj["is_physics_object"];
		
		objects.push_back(make_shared<Object>(resourcePath + meshpath, translation, rotation, scale, isPhysicsObject));
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