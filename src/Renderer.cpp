#include "Renderer.h"
#include <cstdlib>

using namespace std;
using vec3 = glm::vec3;
using vec4 = glm::vec4;

Renderer::Renderer(vector<shared_ptr<Object>>& objectList, string resourceDirectory): objects(objectList), resourceDir(resourceDirectory) {
	initGraphics();
	initScene();
}

void Renderer::render() {
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
	
	
	for (auto obj: objects) {
		obj->draw(MV, bphongProg);
	}
	
	P->popMatrix();
	MV->popMatrix();
	GLSL::checkError(GET_FILE_LINE);
}

void Renderer::initGraphics() {

	// Set error callback.
	glfwSetErrorCallback(errorCallback);

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

	// Set glfw callback functions
	glfwSetKeyCallback(window, keyCallback);
	glfwSetCharCallback(window, charCallback);
	glfwSetCursorPosCallback(window, cursorPosCallback);
	glfwSetMouseButtonCallback(window, mouseCallback);
	glfwSetFramebufferSizeCallback(window, resizeCallback);
	glfwSetScrollCallback(window, scrollCallback);

	// Initial all the object shapes now that the OpenGL context is created
	for (auto obj: objects) {
		obj->shape->init();
	}
}

void Renderer::initScene() {

	// Initialize time.
	glfwSetTime(0.0); 			

	// Set background color.
	glClearColor(0.5f, 0.5f, 0.5f, 0.5f); 	

	// Enable z-buffer test
	glEnable(GL_DEPTH_TEST); 				

	// Initialize shader
	vector<string> attributeNames = {"aPos", "aNor"};
	vector<string> uniformNames  = {"MV", "P", "MVIT", "ka", "kd", "ks", "s", "lightCount", "lightPositions", "lightColors"};
	bphongProg 	= makeProg("bphong", attributeNames, uniformNames);

	// Initialize camera
	camera = make_shared<Camera>();
	camera->setInitDistance(4.5f);

	// Initialize matrix stacks
	P = make_shared<MatrixStack>();
	MV = make_shared<MatrixStack>();

	// Manually initialize lights (for now) 
	lightCount = 2;
	lightColors[0] 		= vec3(0.5, 0.5, 0.5);
	lightPositions[0] 	= vec3(-1, 1, -1);
	lightColors[1] 		= vec3(0.5, 0.5, 0.5);
	lightPositions[1] 	= vec3(1, 1, 1);

	bphongProg->bind(); 
	GLSL::checkError(GET_FILE_LINE);
}

void Renderer::errorCallback(int error, const char *description) {
	cerr << description << endl;
}

void Renderer::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

void Renderer::mouseCallback(GLFWwindow* window, int button, int action, int mods) {
	Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));

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
		renderer->camera->mouseClicked((float)xmouse, (float)ymouse, shift, ctrl, alt);
	}
}

void Renderer::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
	Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if(state == GLFW_PRESS) {
		renderer->camera->mouseMoved((float)xpos, (float)ypos);
	}
}

void Renderer::charCallback(GLFWwindow* window, unsigned int key) {
	Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	
	switch (key) {
		case 'x':
			renderer->PAUSED = !renderer->PAUSED;
			if (renderer->PAUSED) {
				renderer->STEP = false;
			}
			cerr << "Physics simulation: " << (renderer->PAUSED ? "OFF" : "ON") << endl;
			break;

		case 'r':
			renderer->RESET = true;
			renderer->PAUSED = true;
			renderer->STEP = false;
			break;
		
		case 's':
			if (renderer->PAUSED) {
				renderer->STEP = true;
			}
			break;

		case 'c':
			renderer->CULL = !renderer->CULL;
			if (renderer->CULL) {
				glEnable(GL_CULL_FACE);
			} else {
				glDisable(GL_CULL_FACE);
			}
			break;
		
		case 'z':
			renderer->FILL = !renderer->FILL;
			glPolygonMode(GL_FRONT_AND_BACK, renderer->FILL ? GL_FILL : GL_LINE);
			break;
	}
}

void Renderer::resizeCallback(GLFWwindow* window, int width, int height) {
	Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));

	renderer->viewportWidth = width;
	renderer->viewportHeight = height;
	glViewport(0, 0, width, height);
}

void Renderer::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));

	renderer->camera->zoom(yoffset*ZOOM_SPEED);
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




