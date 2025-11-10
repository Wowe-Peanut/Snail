#include "Renderer.h"
#include <cstdlib>

using namespace std;
using vec3 = glm::vec3;

Renderer::Renderer(std::vector<Object>& objectList, std::string resourceDirectory): objects(objectList), resourceDir(resourceDirectory) {
	initWindow();
	initScene();
}

void Renderer::render() {
	// TODO
}

void Renderer::initWindow() {

	// Set error callback.
	glfwSetErrorCallback(staticErrorCallback);

	// Initialize the library.
	if(!glfwInit()) {
		exit(1);
	}

	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, "Ryan O'Mullan", NULL, NULL);
	if(!window) {
		glfwTerminate();
		exit(1);
	}
	glfwMakeContextCurrent(window);
	glfwSetWindowUserPointer(window, this); 


	// Initialize GLEW.
	glewExperimental = true;
	if(glewInit() != GLEW_OK) {
		cerr << "Failed to initialize GLEW" << endl;
		exit(1);
	}
	glGetError(); // A bug in glewInit() causes an error that we can safely ignore.
	// cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
	// cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	GLSL::checkVersion();

	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, staticKeyCallback);
	glfwSetCharCallback(window, staticCharCallback);
	glfwSetCursorPosCallback(window, staticCursorPosCallback);
	glfwSetMouseButtonCallback(window, staticMouseCallback);
	glfwSetFramebufferSizeCallback(window, staticResizeCallback);
}

void Renderer::initScene() {

}



// Some callback functions need access to Renderer member variables. Rather than use a singleton these functions read
// the glfw 'window user pointer' which is set to the Renderer instance in question and calls the appropriate member callback.
void Renderer::staticErrorCallback(int error, const char *description) {
	cerr << description << endl;
}

void Renderer::staticKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

void Renderer::staticMouseCallback(GLFWwindow* window, int button, int action, int mods) {
	Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	renderer->mouseCallback(window, button, action, mods);
}

void Renderer::staticCursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
	Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	renderer->cursorPosCallback(window, xpos, ypos);
}

void Renderer::staticCharCallback(GLFWwindow* window, unsigned int key) {
	Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	renderer->charCallback(window, key);
}

void Renderer::staticResizeCallback(GLFWwindow* window, int width, int height) {
	Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	renderer->resizeCallback(window, width, height);
}

void Renderer::mouseCallback(GLFWwindow* window, int button, int action, int mods) {
	// Get the current mouse position.
	double xmouse, ymouse;
	glfwGetCursorPos(window, &xmouse, &ymouse);
	// Get current window size.
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	if(action == GLFW_PRESS) {
		bool shift = (mods & GLFW_MOD_SHIFT) != 0;
		bool ctrl  = (mods & GLFW_MOD_CONTROL) != 0;
		bool alt   = (mods & GLFW_MOD_ALT) != 0;
		camera.mouseClicked((float)xmouse, (float)ymouse, shift, ctrl, alt);
	}
}

void Renderer::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if(state == GLFW_PRESS) {
		camera.mouseMoved((float)xpos, (float)ypos);
	}
}

void Renderer::charCallback(GLFWwindow* window, unsigned int key) {

	switch (key) {
		case 'n':
			PAUSED = !PAUSED;
			cerr << "Physics simulation: " << (PAUSED ? "ON" : "OFF") << endl;
			break;

		case 'c':
			CULL = !CULL;
			if (CULL) {
				glEnable(GL_CULL_FACE);
			} else {
				glDisable(GL_CULL_FACE);
			}
			break;
		
		// Toggle triangle full OR wireframe
		case 'z':
			FILL = !FILL;
			glPolygonMode(GL_FRONT_AND_BACK, FILL ? GL_FILL : GL_LINE);
			break;
	}
}

void Renderer::resizeCallback(GLFWwindow* window, int width, int height) {
	viewportWidth = width;
	viewportHeight = height;
	glViewport(0, 0, width, height);
}

shared_ptr<Program> Renderer::makeProg(string name, vector<string> attributeNames, vector<string> uniformNames) {
	auto prog = make_shared<Program>();
	prog->setShaderNames(resourceDir + "shaders/" + name + "_vert.glsl", resourceDir + "shaders/" + name + "_frag.glsl");
	prog->setVerbose(true);
	prog->init();

	for (string attrib: attributeNames) prog->addAttribute(attrib);
	for (string uniform: uniformNames) prog->addUniform(uniform);
	
	prog->setVerbose(false);
	return prog;
}




