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
float constexpr A0					= 1.0;
float constexpr A1					= 0.0429;
float constexpr A2					= 0.9857;

// 3D Models
map<string, shared_ptr<Shape>> models;

// Shaders
shared_ptr<Program> 			p1DefaultProg; 	
shared_ptr<Program>				p1SORProg;
shared_ptr<Program>				p2BPhongProg;

// Objects
vector<shared_ptr<Object>> 		bunnies;
vector<shared_ptr<Object>>		lights;
shared_ptr<Object>				floorPlane;

// Deffered Rendering
int textureWidth = DEFAULT_WIDTH;
int textureHeight = DEFAULT_HEIGHT;
GLuint framebufferID;
GLuint posTexture;
GLuint norTexture;
GLuint keTexture;
GLuint kdTexture;

bool keyToggles[256] = {false}; // only for English keyboards!

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
		case 'b':
			blurOn = !blurOn;
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
	
	prog->bind();
	glUniform1f(prog->getUniform("A0"), A0);
	glUniform1f(prog->getUniform("A1"), A1);
	glUniform1f(prog->getUniform("A2"), A2);

	glUniform1i(prog->getUniform("posTexture"), 0);
	glUniform1i(prog->getUniform("norTexture"), 1);
	glUniform1i(prog->getUniform("keTexture"), 2);
	glUniform1i(prog->getUniform("kdTexture"), 3);
	prog->unbind();

	prog->setVerbose(false);
	return prog;
}

void initTexture(GLuint& texture, GLenum attachment) {
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, textureWidth, textureHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture, 0);
}

void initFrameBuffer() {
	// Generate off-screen frame buffer	
	glGenFramebuffers(1, &framebufferID);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
	
	// Initialize textures and depth buffer
	initTexture(posTexture, GL_COLOR_ATTACHMENT0);
	initTexture(norTexture, GL_COLOR_ATTACHMENT1);
	initTexture(keTexture, GL_COLOR_ATTACHMENT2);
	initTexture(kdTexture, GL_COLOR_ATTACHMENT3);

	// Initialize depth buffer
	GLuint depthrenderbuffer;
	glGenRenderbuffers(1, &depthrenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, textureWidth, textureHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);
	
	// Assign textures as the off-screen frame buffers output
	GLenum attachments[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
	glDrawBuffers(4, attachments);
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		cerr << "Framebuffer is not ok" << endl;
		exit(1);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// If the window is resized, capture the new size and reset the viewport
static void resize_callback(GLFWwindow *window, int width, int height) {
	textureWidth = width;
	textureHeight = height;
	initFrameBuffer();
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
	vector<string> uniformNames  = {"MV", "P", "MVIT", "ke", "kd", "ks", "s", "lightCount", 
									"lightPositions", "lightColors", "A0", "A1", "A2", "t",
									"posTexture", "norTexture", "keTexture", "kdTexture", 
									"windowSize", "blurOn"};
	p1DefaultProg 	= makeProg("default_p1", attributeNames, uniformNames);
	p1SORProg 		= makeProg("SOR_p1", attributeNames, uniformNames);
	p2BPhongProg 	= makeProg("bphong_p2", attributeNames, uniformNames);
	// --------------------------------------------------------------------------



	// Initialize the off-screen frame buffer -----------------------------------
	initFrameBuffer();
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



	// World objects: spaced according to OBJECT_SPACING and OBJECT_GRID_SIZE ---
	srand(glfwGetTime());
	float yrot = randf() * 2*M_PI;
	bunnies.push_back(make_shared<Object>(models["bunny"], vec3(0,0,0), vec3(0,yrot,0), vec3(rfrange(0.3, .5)), vec3(0)));

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





void drawScene(shared_ptr<MatrixStack> MV, shared_ptr<MatrixStack> P) {
	float t = glfwGetTime();
	vec3 lightPositions[NUM_LIGHTS];
	vec3 lightColors[NUM_LIGHTS];


	// Rotate lights and load attributes into arrays
	for (size_t li=0; li<NUM_LIGHTS; li++) {
		lights.at(li)->worldRotation(LIGHT_ROT_SPEED * (li%2 ? -1 : 1)); 	// Alternating rotation direction
		lights.at(li)->translation.y = 1 + 0.2*sin(t + li*li);				// Offset vertical oscillation

		lightPositions[li] = vec3(MV->topMatrix() * vec4(lights.at(li)->translation, 1));
		lightColors[li] = lights.at(li)->ke;
	}



	
	///////////////////////////////////////////////////////////
	// FIRST PASS - RENDER TO TEXTURE
	///////////////////////////////////////////////////////////	
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
	glViewport(0, 0, textureWidth, textureHeight);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	// Default Pass 1 Shader -------------------------------------------------------
	p1DefaultProg->bind();

	glUniformMatrix4fv(p1DefaultProg->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));

	// Draw floating lights
	for (auto light: lights) {
		light->draw(MV, p1DefaultProg);
	}

	// Draw spinning bunnies	
	for (size_t i=0; i<bunnies.size(); i++) {
		auto bunny = bunnies.at(i);
		
		bunny->rotation.y = (t + 5*i) * (i%2 ? -1 : 1);	// Offset rotation phase and alternating direction
		bunny->draw(MV,p1DefaultProg);
	}
	
	floorPlane->draw(MV, p1DefaultProg);
	p1DefaultProg->unbind();
	// -----------------------------------------------------------------------------


	///////////////////////////////////////////////////////////
	// SECOND PASS - RENDER TO SCREEN
	///////////////////////////////////////////////////////////	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glm::mat4 projectionMatrix = glm::ortho(-0.5f, 0.5f, -0.5f, 0.5f, 0.01f, 10.0f);
	glm::vec2 windowSize;

	int width, height;
    glfwGetFramebufferSize(window, &width, &height);
	windowSize = glm::vec2(width, height); 
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// Clear the previous ModelView and projection matrices (use ortho for projection, and identity for ModelView)
	MV->pushMatrix();
	MV->loadIdentity();
	MV->multMatrix(projectionMatrix);	
	
	// Rotate plane (the tv screen) to be directly in front of camera
	MV->translate(0, 0, -1);
	MV->rotate(M_PI/2, 1, 0, 0);
	
	// Bind pass-2 shader and assign uniforms
	p2BPhongProg->bind();
	glUniformMatrix4fv(p2BPhongProg->getUniform("MV"), 1, GL_FALSE, value_ptr(MV->topMatrix()));
	glUniformMatrix4fv(p2BPhongProg->getUniform("P"), 1, GL_FALSE, value_ptr(projectionMatrix));
	glUniform3fv(p2BPhongProg->getUniform("lightPositions"), NUM_LIGHTS, value_ptr(lightPositions[0]));
	glUniform3fv(p2BPhongProg->getUniform("lightColors"), NUM_LIGHTS, value_ptr(lightColors[0]));
	glUniform1i(p2BPhongProg->getUniform("lightCount"), NUM_LIGHTS); 
	glUniform2fv(p2BPhongProg->getUniform("windowSize"), 1, value_ptr(windowSize));
	glUniform1i(p2BPhongProg->getUniform("blurOn"), int(blurOn));
	
	// Bind the textures that will be used
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, posTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, norTexture);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, keTexture);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, kdTexture);
	
	models["plane"]->draw(p2BPhongProg);
	p2BPhongProg->unbind();
	MV->popMatrix();
}

// This function is called every frame to draw the scene.
static void render() {
	// Clear framebuffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

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
	drawScene(MV, P);

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
