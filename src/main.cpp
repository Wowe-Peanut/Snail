
#include "render_engine.h"
#include "physics_engine.h"
#include "parser.h"
#include <unistd.h>
using namespace std;

const double MICROSECONDS = 1000000.0;

int main(int argc, char **argv) {
	if (argc < 3) {
		cerr << "BAD USAGE - SIM RESOURCEDIR JSON" << endl;
		return 1;
	}

	string resourcePath = argv[1] + string("/");
	string jsonPath = argv[2];

	vector<shared_ptr<Object>> objects = parseObjects(resourcePath, jsonPath);
	SimParameters params = parseParameters(resourcePath + jsonPath);

	RenderEngine renderer(objects, resourcePath);
	PhysicsEngine engine(objects, params);
	
	while (!glfwWindowShouldClose(renderer.window)) {
		if (renderer.PAUSED) {
			if (renderer.STEP) {
				engine.step();
				renderer.STEP = false;
			}
		} else {
			engine.step();
		}

		if (renderer.RESET) {
			engine.reset();
			renderer.RESET = false;
		}

		renderer.render();
		glfwSwapBuffers(renderer.window);
		glfwPollEvents();

		usleep(engine.params.dt * MICROSECONDS);
	}

	renderer.bphongProg->unbind();
	glfwDestroyWindow(renderer.window);
	glfwTerminate();
}