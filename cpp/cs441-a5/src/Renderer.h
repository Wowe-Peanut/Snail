#pragma once
#include <iostream>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.h"
#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Shape.h"
#include "Object.h"

#define MAX_LIGHTS 10
#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600

class Renderer {
	public:
		GLFWwindow* window;

		Renderer(std::vector<std::shared_ptr<Object>>& objectList, std::string resourceDirectory);
		void init();
		void render();

		// GLFW member callback functions (some callbacks require Renderer member variables)
		void mouseCallback(GLFWwindow* window, int button, int action, int mods);
		void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
		void charCallback(GLFWwindow* window, unsigned int key);
		void resizeCallback(GLFWwindow* window, int width, int height);

		// GLFW static callback functions
		static void staticErrorCallback(int error, const char *description);
		static void staticKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void staticMouseCallback(GLFWwindow* window, int button, int action, int mods);
		static void staticCursorPosCallback(GLFWwindow* window, double xpos, double ypos);
		static void staticCharCallback(GLFWwindow* window, unsigned int key);
		static void staticResizeCallback(GLFWwindow* window, int width, int height);

	private:

		bool PAUSED = true;
		bool CULL = true;
		bool FILL = true;

		int viewportWidth;
		int viewportHeight;

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
		void initWindow();
		void initScene();


};