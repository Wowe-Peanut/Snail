#pragma once

#include "camera.h"
#include "matrix_stack.h"
#include "program.h"
#include "shape.h"
#include "object.h"
#include "glsl.h"

#include <iostream>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

float constexpr 	DEFAULT_WIDTH = 800;
float constexpr 	DEFAULT_HEIGHT = 600;
int constexpr	 	MAX_LIGHTS = 10;
float constexpr		ZOOM_SPEED = 0.5;

class Renderer {
	public:
		GLFWwindow* window;

		Renderer(std::vector<std::shared_ptr<Object>>& objectList, std::string resourceDirectory);
		void render();
		void initGraphics();
		void initScene();

		// GLFW callback functions
		static void errorCallback(int error, const char *description);
		static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void mouseCallback(GLFWwindow* window, int button, int action, int mods);
		static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
		static void charCallback(GLFWwindow* window, unsigned int key);
		static void resizeCallback(GLFWwindow* window, int width, int height);
		static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

		bool PAUSED = true;
		bool CULL = true;
		bool FILL = true;
		bool STEP = false;
		bool RESET = false;

		int viewportWidth = DEFAULT_WIDTH;
		int viewportHeight = DEFAULT_HEIGHT;

		std::vector<std::shared_ptr<Object>> objects;
		
		std::string resourceDir = "./";
		std::shared_ptr<Program> bphongProg;
		glm::vec3 lightPositions[MAX_LIGHTS];
		glm::vec3 lightColors[MAX_LIGHTS];
		int lightCount;

		std::shared_ptr<Camera> camera;
		std::shared_ptr<MatrixStack> P;
		std::shared_ptr<MatrixStack> MV;

		// Helper functions
		std::shared_ptr<Program> makeProg(std::string name, std::vector<std::string> attributeNames, std::vector<std::string> uniformNames);
};