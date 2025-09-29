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

// Other
GLFWwindow *window; 
shared_ptr<Camera> camera;
string RESOURCE_DIR = "./";
bool OFFLINE = false;
bool blurOn = false;

// Parameters
float constexpr DEFAULT_WIDTH		= 800;
float constexpr DEFAULT_HEIGHT		= 600;

float constexpr FLOOR_SIZE 			= 3;

int   constexpr NUM_LIGHTS 			= 1;
float constexpr LIGHT_ROT_SPEED		= 0.001;
float constexpr MAX_LIGHT_RADIUS	= 1;

// 3D Models
map<string, shared_ptr<Shape>> models;

// Shaders
shared_ptr<Program> 			bphongProg; 	

// Objects
vector<shared_ptr<Object>> 		bunnies;
vector<shared_ptr<Object>>		lights;
shared_ptr<Object>				floorPlane;

// Current window dimensions
int textureWidth = DEFAULT_WIDTH;
int textureHeight = DEFAULT_HEIGHT;

// only for English keyboards!
bool keyToggles[256] = {false}; 
bool culling = true;
bool fillTriangles = true;

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


// https://lencerf.github.io/post/2019-09-21-save-the-opengl-rendering-to-image-file/
static void saveImage(const char *filepath, GLFWwindow *w) {
	int width, height;
	glfwGetFramebufferSize(w, &width, &height);
	GLsizei nrChannels = 3;
	GLsizei stride = nrChannels * width;
	stride += (stride % 4) ? (4 - stride % 4) : 0;
	GLsizei bufferSize = stride * height;
	std::vector<char> buffer(bufferSize);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
	stbi_flip_vertically_on_write(true);
	int rc = stbi_write_png(filepath, width, height, nrChannels, buffer.data(), stride);
	if(rc) {
		cout << "Wrote to " << filepath << endl;
	} else {
		cout << "Couldn't write to " << filepath << endl;
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
	textureWidth = width;
	textureHeight = height;
	glViewport(0, 0, width, height);
}

float randf() {
	return ((float) rand()) / RAND_MAX;  
}

float rfrange(float lo, float hi) {
	return lo + randf()*(hi-lo);	
}


// This function is called once to initialize the scene and OpenGL
static void init() {
	glfwSetTime(0.0); 						// Initialize time.
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 	// Set background color.
	glEnable(GL_DEPTH_TEST); 				// Enable z-buffer test
	


	// Shader programs ----------------------------------------------------------
	vector<string> attributeNames = {"aPos", "aNor"};
	vector<string> uniformNames  = {"MV", "P", "MVIT", "ke", "kd", "ks", "s", "lightCount", "lightPositions", 
									"lightColors", "posTexture", "norTexture", "keTexture", "kdTexture"};
	bphongProg 	= makeProg("bphong", attributeNames, uniformNames);
	// --------------------------------------------------------------------------




	// Camera -------------------------------------------------------------------
	camera = make_shared<Camera>();
	camera->setInitDistance(2.0f);
	// --------------------------------------------------------------------------



	// 3D Models (from file) ----------------------------------------------------
	vector<string> modelNames = {"bunny", "plane", "teapot"};
	for (string modelName: modelNames) {
		shared_ptr<Shape> model = make_shared<Shape>();
		model->loadMeshFile(RESOURCE_DIR + "models/" + modelName + ".obj");
		model->init();

		models.insert({modelName, model});
	}

	models.insert({"sphere", Shape::buildSphere(20)});
	models.insert({"sor", Shape::buildSOR(20)});
	// --------------------------------------------------------------------------



	// World objects ------------------------------------------------------------
	srand(glfwGetTime());
	float yrot = randf() * 2*M_PI;
	bunnies.push_back(make_shared<Object>(models["sphere"], vec3(0,0,0), vec3(0,yrot,0), vec3(rfrange(0.3, .5)), vec3(0)));

	// --------------------------------------------------------------------------



	// Lights -------------------------------------------------------------------
	for (int li=0; li<NUM_LIGHTS; li++) {
		float theta = randf() * 2*M_PI;
		float r = randf() * MAX_LIGHT_RADIUS * MAX_LIGHT_RADIUS;
		vec3 position = vec3(sqrt(r)*cos(theta), 1, sqrt(r)*sin(theta)); // sqrt(r) used to ensure uniform distribution in cartesian
		vec3 emissive = vec3(randf(), randf(), randf());
		
		shared_ptr<Object> light = make_shared<Object>(models["sphere"], position,  vec3(0), vec3(0.04), emissive);
		light->kd = vec3(0);
		light->ks = vec3(0);	
		lights.push_back(light);
	}
	// --------------------------------------------------------------------------



	// Floor plane object -------------------------------------------------------
	floorPlane = make_shared<Object>(models["plane"], vec3(0), vec3(0), vec3(FLOOR_SIZE, 1, FLOOR_SIZE), vec3(0));
	floorPlane->kd = vec3(1);	
	// --------------------------------------------------------------------------



	// Debugging assertion (put this after an GL call to check for problems)
	GLSL::checkError(GET_FILE_LINE);
}


// This function is called every frame to draw the scene.
static void render() {
	// Clear framebuffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	// Set camera aspect ratio
	float aspect = width / (float) height;
	camera->setAspect(aspect);

	// Matrix stacks
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();

	// ------------------------------------------------------
	P->pushMatrix();
	MV->pushMatrix();

	camera->applyProjectionMatrix(P);
	camera->applyViewMatrix(MV);	

	vec3 lightPositions[NUM_LIGHTS];
	vec3 lightColors[NUM_LIGHTS];


	// Load light attributes into buffers to send to GPU
	for (size_t li=0; li<NUM_LIGHTS; li++) {
		lightPositions[li] = vec3(MV->topMatrix() * vec4(lights.at(li)->translation, 1));
		lightColors[li] = lights.at(li)->ke;
	}

	// Setup OpenGL viewport and buffers
	glViewport(0, 0, textureWidth, textureHeight);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	// Bphong Shader ---------------------------------------------------------------
	bphongProg->bind();

	glUniformMatrix4fv(bphongProg->getUniform("MV"), 1, GL_FALSE, value_ptr(MV->topMatrix()));
	glUniformMatrix4fv(bphongProg->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
	glUniform3fv(bphongProg->getUniform("lightPositions"), NUM_LIGHTS, value_ptr(lightPositions[0]));
	glUniform3fv(bphongProg->getUniform("lightColors"), NUM_LIGHTS, value_ptr(lightColors[0]));
	glUniform1i(bphongProg->getUniform("lightCount"), NUM_LIGHTS); 

	for (auto bunny: bunnies) {
		bunny->draw(MV, bphongProg);
	}
	
	floorPlane->draw(MV, bphongProg);

	bphongProg->unbind();
	// -----------------------------------------------------------------------------

	P->popMatrix();
	MV->popMatrix();
	// ------------------------------------------------------
	
	// Save output to image if in offline mode	
	GLSL::checkError(GET_FILE_LINE);
	if(OFFLINE) {
		string filename = "output.png";
		saveImage(filename.c_str(), window);
		GLSL::checkError(GET_FILE_LINE);
		glfwSetWindowShouldClose(window, true);
	}
}

int main(int argc, char **argv) {
	if(argc < 2) {
		cout << "Usage: A3 RESOURCE_DIR" << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");
	
	// Optional argument
	if(argc >= 3) {
		OFFLINE = atoi(argv[2]) != 0;
	}

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
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
