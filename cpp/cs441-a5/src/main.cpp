#include <iostream>

#include "Renderer.h"
#include "PhysicsEngine.h"
#include "Object.h"
#include "Shape.h"

#include <fstream>
#include "json.hpp"

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

vector<shared_ptr<Object>> parseObjects(json objectListJson) {
	vector<shared_ptr<Object>> objects;

	for (auto obj: objectListJson) {
		string shape = obj["shape"];
		json transform = obj["transformation"];
		vec3 translation = jsontovec3(transform["translation"]);
		vec3 scale = jsontovec3(transform["scale"]);
		vec3 rotation = jsontovec3(transform["rotation"]);
		bool isPhysicsObject = obj["is_physics_object"];
		

		// Shape specific initializiation
		if (shape == "cube") {
			double segLen = obj["segment_length"];
			int segNum = obj["segments"];
			
			objects.push_back(make_shared<Object>(Shape::buildCube(segLen, segNum), translation, rotation, scale, isPhysicsObject));
		}
		else if (shape == "plane") {
			double segLen = obj["segment_length"];
			int segNum = obj["segments"];

			objects.push_back(make_shared<Object>(Shape::buildPlane(segLen, segNum), translation, rotation, scale, isPhysicsObject));
		}
		else {
			cout << "UNKNOWN SHAPE IN TARGET JSON" << endl;
		}

	}
	
	return objects;
}

void simulate(string resourcePath, string jsonPath) {
	json data = openjson(resourcePath + jsonPath);

	// renderer must be initialized before objects can be created - opengl must be setup before objects can create their buffers
	Renderer renderer(resourcePath);
	vector<shared_ptr<Object>> objects = parseObjects(data["objects"]);
	renderer.initScene(objects);

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

	// Read in cmdargs
	// -----------------------------------------------------------
	if (argc < 4) {
		cerr << "BAD USAGE - A5 RESOURCEDIR MODE JSON" << endl;
		return 1;
	}

	string resourcePath = argv[1] + string("/");
	string mode = argv[2];
	string jsonPath = argv[3];

	// Mode specific calls
	// -----------------------------------------------------------
	if (mode == "-r" || mode == "--replay") {
		cerr << "REPLAY NOT IMLEMENTED YET" << endl;
	} else if (mode == "-s" || mode == "--simulate") {
		simulate(resourcePath, jsonPath);
	} else {
		cerr << "'" << mode << "'" << "is not a valid mode" << endl;
		return 1;
	}

	return 0;
}