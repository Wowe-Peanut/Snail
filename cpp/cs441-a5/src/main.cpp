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
		

		// Shape specific initializiation
		if (shape == "cube") {
			double segLen = obj["segmentLength"];
			int segNum = obj["segments"];
			
			objects.push_back(make_shared<Object>(Shape::buildCube(segLen, segNum), translation, rotation, scale, true));
		}
		else if (shape == "plane") {
			double segLen = obj["segmentLength"];
			int segNum = obj["segments"];

			objects.push_back(make_shared<Object>(Shape::buildPlane(segLen, segNum), translation, rotation, scale, false));
		}
		else {
			cout << "UNKNOWN SHAPE IN TARGET JSON" << endl;
		}

	}
	
	return objects;
}

void simulate(string resourcePath, string jsonPath) {
	Renderer renderer(resourcePath);
	// PhysicsEngine engine();
	json data = openjson(resourcePath + jsonPath);

	// initGraphics must come before any objects - opengl must be initialized before objects can create their buffers
	renderer.initGraphics(); 
	vector<shared_ptr<Object>> objects = parseObjects(data["objects"]);
	renderer.initScene(objects);

	
	while (!glfwWindowShouldClose(renderer.window)) {
		renderer.render();
		glfwSwapBuffers(renderer.window);
		glfwPollEvents();
	}
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


/** 
 * My goal is to be able to call like 
 * './A5 ../resources -r replay.txt' and have it replay the animation or
 * or something like
 * './A5 ../resources -s setup.json
 * 
 * It should be able to:
 * 		Replay 
 * 		Simulate (with options to 'render & save', 'save only', 'render only')
 * 			'saving' should be a world-state-frames not image-frames (that way you can still move camera around in replay mode)
 * 				and it should only save visibile triangles positions & normals (it don't care about internal shit)
 * 			Saving will require setting a range of time to simulate 
 * 		
 * 		If replaying or simulating with 'render' you should be able to interact with animation:
 * 			- Toggle pausing, culling, single step forward, reseting
 * 			- Camera should be able to rotate around center, zoom in/out
 * 			- It should also always start out as 'paused'
 * 
 * When we are simulating but only 'saving' and not 'rendering', it shouldn't interact with the opengl shit at all
 * for speed and separation purposes. That means I need a way to easily link the two systems so that is quite efficient 
 * while running in the case where I want to do both.
 * 
 * So I think that there should be a PhysicsEngine class that can be initialized with all of the appropriate 
 * constants (h, tol, maxiter, etc) that DOES NOT belong to the Renderer. These should be kept separate
 * 
 * These parameter should be set in the json file input file. The only concern I have now is with interactions between
 * objects. It seems like the PhysicsEngine will need to be passed references to all the objects and initialized by being given
 * which objects are allowed to collide and how (how to tell it is tbh).
 * 
 * For that to work with some of the other things I have in mind, I need to modify the object & shape class to keep visibile and hidden
 * parts of their mesh separate. Since renderer should only be drawing the external triangles and the object needs a way to take
 * the values it gets from the physics simulation and copy just the outer triangles to the renderes buffer. But for now I'll keep 
 * the renderer drawing everything as it is and change the object class later
 * 
 * 
 * Flow should be like this:
 * 	main:
 * 		takes in cmdargs 
 *
 * 		*Both the Renderer and/or PhysicsEngine class should be passed this list of constructed objects b/c they are SEPARATE ENTITIES*
 * 
 * 		if 'simulate':
 * 			reads setup json and constructs list of objects specified
 * 			construct PhysicsEngine
 * 			if 'render': construct Renderer
 * 
 * 			while running:
 * 				if 'render', renderer renders shit
 * 				if 'save', take snapshot and write to file
 * 				engine steps step
 * 
 * 		if 'replay':
 * 			reset save json and construct list of objects specified
 * 			construct Renderer
 * 			call Renderer.playback
 *
 */