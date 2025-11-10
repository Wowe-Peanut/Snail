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


vec3 jtovec3 (json jsonlist) {
	return vec3(jsonlist[0], jsonlist[1], jsonlist[2]);
}

vector<Object> parseObjects(json objectListJson) {
	vector<Object> objects;

	for (json obj: objectListJson["objects"]) {

		string shape = obj["shape"];
		json transform = obj["transformation"];
		vec3 translation = jtovec3(transform["translation"]);
		vec3 scale = jtovec3(transform["scale"]);
		vec3 rotation = jtovec3(transform["rotation"]);
		

		// Shape specific initializiation
		if (shape == "cube") {
			double segLen = obj["segmentLength"];
			int segNum = obj["segments"];
			
			objects.emplace_back(Shape::buildCube(segLen, segNum), translation, rotation, scale, true);
		}
		else if (shape == "plane") {
			double segLen = obj["segmentLength"];
			int segNum = obj["segments"];

			objects.emplace_back(Shape::buildPlane(segLen, segNum), translation, rotation, scale, true);
		}
		else {
			cout << "UNKNOWN SHAPE IN TARGET JSON" << endl;
		}

	}
	
	return objects;
}

void replay(vector<Object>& objects, json parameters) {
	cerr << "REPLAY NOT IMLEMENTED YET" << endl;
}

void simulate(vector<Object>& objects, json parameters) {
	Renderer renderer(objects);
	PhysicsEngine engine(objects, parameters["deltat"], parameters["tol"], parameters["maxiter"]);

	
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



	// Parse Input Json 
	// -----------------------------------------------------------
	ifstream f(resourcePath + jsonPath);
	json data;
	

	if (!f.is_open()) {
		cerr << "Failed to open JSON file" << endl;
		return 1;
	}

	try {
		json data = json::parse(f);
	} catch (json::parse_error& ex) {
		cerr << "JSON Parse error at byte " << ex.byte << endl;
		return 1;
	} 

	vector<Object> objects = parseObjects(data["objects"]);
	json parameters = data["parameters"];



	// Mode specific calls
	// -----------------------------------------------------------
	if (mode == "-r" || mode == "--replay") {
		replay(objects, parameters);
	} else if (mode == "-s" || mode == "--simulate") {
		simulate(objects, parameters);
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