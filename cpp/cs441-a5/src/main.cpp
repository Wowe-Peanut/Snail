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
using namespace std;

int main(int argc, char **argv) {
	if (argc < 4) {
		cout << "BAD USAGE - NOT ENOUGH ARGS" << endl;
		return 0;
	}

	string resourceDir = argv[1] + string("/");
	string mode = argv[2];
	string scenejson = argv[3];

	if (mode == "-r" || mode == "--replay") {
		cout << "REPLAY - NOT IMLEMENTED YET" << endl;
	} else if (mode == "-s" || mode == "--simulate") {
		

		ifstream f(resourceDir + scenejson);
		json data = json::parse(f);
		
		vector<Object> objects;
		for (json obj: data["objects"]) {
			string shape = obj["shape"];

			if (shape == "cube") {
				double segLen = obj["segmentLength"];
				int segNum = obj["segments"];
				json trans = obj["transformation"]["translation"];

				objects.emplace_back(Shape::buildCube(segLen, segNum), glm::vec3(trans[0], trans[1], trans[2]), glm::vec3(0), glm::vec3(1), true);

			}
			else if (shape == "plane") {

			}
			else {
				cout << "UNKNOWN SHAPE IN TARGET JSON" << endl;
				return 0;
			}

		}


	}

	return 0;
}
