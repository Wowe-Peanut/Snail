#include <cassert>
#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <random>
#include <map>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Camera.h"
#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Shape.h"
#include "Object.h"

using namespace std;
using glm::vec3, glm::vec4;


// Parameters
float constexpr DEFAULT_WIDTH		= 800;
float constexpr DEFAULT_HEIGHT		= 600;
float constexpr FLOOR_SIZE 			= 3;
int   constexpr MAX_LIGHTS 			= 10;
float constexpr MAX_LIGHT_RADIUS	= 1;


// Global Scene Variables
GLFWwindow *window; 
shared_ptr<Camera> camera;
map<string, shared_ptr<Shape>> models;
auto P = make_shared<MatrixStack>();
auto MV = make_shared<MatrixStack>();


shared_ptr<Program> bphongProg; 	
vector<shared_ptr<Object>> worldObjects;
int lightCount = 0;
vec3 lightPositions[MAX_LIGHTS];
vec3 lightColors[MAX_LIGHTS];

string RESOURCE_DIR = "./";
int viewportWidth = DEFAULT_WIDTH;
int viewportHeight = DEFAULT_HEIGHT;
bool culling = true;
bool fillTriangles = true;
bool keyToggles[256] = {false}; 


// This function is called when a GLFW error occurs
static void error_callback(int error, const char *description) {
	cerr << description << endl;
}

// This function is called when a key is pressed
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

// This function is called when the mouse is clicked
static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
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
		camera->mouseClicked((float)xmouse, (float)ymouse, shift, ctrl, alt);
	}
}

// This function is called when the mouse moves
static void cursor_position_callback(GLFWwindow* window, double xmouse, double ymouse) {
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if(state == GLFW_PRESS) {
		camera->mouseMoved((float)xmouse, (float)ymouse);
	}
}

// This function is called when a char is typed
static void char_callback(GLFWwindow *window, unsigned int key) {
	keyToggles[key] = !keyToggles[key];

	switch (key) {

		// Toggle triangle culling
		case 'c':
			culling = !culling;
			if (culling) {
				glEnable(GL_CULL_FACE);
			} else {
				glDisable(GL_CULL_FACE);
			}
			break;
		
		// Toggle triangle full OR wireframe
		case 'z':
			fillTriangles = !fillTriangles;
			if (fillTriangles) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			} else {
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			}
			break;
	}
}

// Creates and initializes a shader program with the passed values and assigns attenuation constants
shared_ptr<Program> makeProg(string name, vector<string> attributeNames, vector<string> uniformNames) {
	auto prog = make_shared<Program>();
	prog->setShaderNames(RESOURCE_DIR + "shaders/" + name + "_vert.glsl", RESOURCE_DIR + "shaders/" + name + "_frag.glsl");
	prog->setVerbose(true);
	prog->init();

	for (string attrib: attributeNames) prog->addAttribute(attrib);
	for (string uniform: uniformNames) prog->addUniform(uniform);
	
	prog->setVerbose(false);
	return prog;
}

// If the window is resized, capture the new size and reset the viewport
static void resize_callback(GLFWwindow *window, int width, int height) {
	viewportWidth = width;
	viewportHeight = height;
	glViewport(0, 0, width, height);
}

// This function is called once to initialize the scene and OpenGL
static void init() {
	glfwSetTime(0.0); 						// Initialize time.
	glClearColor(0.5f, 0.5f, 0.5f, 0.5f); 	// Set background color.
	glEnable(GL_DEPTH_TEST); 				// Enable z-buffer test
	


	// Shader programs 
	vector<string> attributeNames = {"aPos", "aNor"};
	vector<string> uniformNames  = {"MV", "P", "MVIT", "ka", "kd", "ks", "s", "lightCount", "lightPositions", "lightColors"};
	bphongProg 	= makeProg("bphong", attributeNames, uniformNames);
	// --------------------------------------------------------------------------

	// Camera 
	camera = make_shared<Camera>();
	camera->setInitDistance(4.0f);
	// --------------------------------------------------------------------------



	// 3D Models 
	vector<string> modelNames = {"bunny", "plane", "teapot"};
	for (string modelName: modelNames) {
		shared_ptr<Shape> model = make_shared<Shape>();
		model->loadMeshFile(RESOURCE_DIR + "models/" + modelName + ".obj");
		model->init();

		models.insert({modelName, model});
	}
	
	models.insert({"sphere", Shape::buildSphere(20)});
	models.insert({"cube", Shape::buildCube(1, 1)});
	// --------------------------------------------------------------------------




	// World objects
	worldObjects = vector<shared_ptr<Object>>();
	worldObjects.push_back(make_shared<Object>(models["cube"], vec3(-0.5, 0.5, -0.5), vec3(0), vec3(1), true));
	worldObjects.push_back(make_shared<Object>(models["plane"], vec3(0), vec3(0), vec3(FLOOR_SIZE, 1, FLOOR_SIZE)));
	// --------------------------------------------------------------------------
	

	// Lights 
	lightCount = 2;

	lightColors[0] 		= vec3(0.5, 0.5, 0.5);
	lightPositions[0] 	= vec3(-1, 1, -1);

	lightColors[1] 		= vec3(0.5, 0.5, 0.5);
	lightPositions[1] 	= vec3(1, 1, 1);

	assert(lightCount <= MAX_LIGHTS);
	// --------------------------------------------------------------------------



	

	// Bind Blinn-Phong Shader (currently the only shader so only need to bind once) 
	bphongProg->bind(); 
	GLSL::checkError(GET_FILE_LINE);
}

// This function is called every frame to draw the scene.
static void render() {

	// Clear color & depth buffers, enable depth test, and set viewport size
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, viewportWidth, viewportHeight);

	// Set camera aspect ratio
	camera->setAspect(viewportWidth / (float) viewportHeight);

	// Apply camera transforms to P and MV
	P->pushMatrix();
	MV->pushMatrix();
	camera->applyProjectionMatrix(P);
	camera->applyViewMatrix(MV);	


	// Transform and load light positions into buffer to send to GPU
	vec3 transformedLightPositions[MAX_LIGHTS];
	for (int li=0; li<lightCount; li++) {
		transformedLightPositions[li] = MV->topMatrix() * vec4(lightPositions[li], 1);
	}


	// Send uniforms to GPU
	glUniformMatrix4fv(bphongProg->getUniform("MV"), 1, GL_FALSE, value_ptr(MV->topMatrix()));
	glUniformMatrix4fv(bphongProg->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
	glUniform1i(bphongProg->getUniform("lightCount"), lightCount); 
	glUniform3fv(bphongProg->getUniform("lightPositions"), lightCount, value_ptr(transformedLightPositions[0]));
	glUniform3fv(bphongProg->getUniform("lightColors"), lightCount, value_ptr(lightColors[0]));
	

	for (auto worldObject: worldObjects) {
		if (worldObject->physicsObject) worldObject->stepForward();
		worldObject->draw(MV, bphongProg);
	}


	
	// -----------------------------------------------------------------------------

	P->popMatrix();
	MV->popMatrix();
	// ------------------------------------------------------
	

	GLSL::checkError(GET_FILE_LINE);
}

int main(int argc, char **argv) {
	if(argc < 2) {
		cout << "Usage: A3 RESOURCE_DIR" << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");
	
	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if(!glfwInit()) {
		return -1;
	}
	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, "Ryan O'Mullan", NULL, NULL);
	if(!window) {
		glfwTerminate();
		return -1;
	}
	// Make the window's context current.
	glfwMakeContextCurrent(window);
	// Initialize GLEW.
	glewExperimental = true;
	if(glewInit() != GLEW_OK) {
		cerr << "Failed to initialize GLEW" << endl;
		return -1;
	}
	glGetError(); // A bug in glewInit() causes an error that we can safely ignore.
	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	GLSL::checkVersion();

	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, key_callback);
	// Set char callback.
	glfwSetCharCallback(window, char_callback);
	// Set cursor position callback.
	glfwSetCursorPosCallback(window, cursor_position_callback);
	// Set mouse button callback.
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	// Set the window resize call back.
	glfwSetFramebufferSizeCallback(window, resize_callback);
	// Initialize scene.
	init();
	// Loop until the user closes the window.
	while(!glfwWindowShouldClose(window)) {
		// Render scene.
		render();
		// Swap front and back buffers.
		glfwSwapBuffers(window);
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	bphongProg->unbind();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
