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
#define DEFAULT_WIDTH 800;
#define DEFAULT_HEIGHT 600;

class Renderer {
	public:
		Renderer(vector<Object>& obj);
		~Renderer();

		void init();
		void playback();

	private:

		unsigned int toggles;
		enum class ViewingToggles {
			CULLING 			= 1 << 0, 
			FILL_TRIANGLES 		= 1 << 1,
			PAUSED				= 1 << 2,
		};

		GLFWwindow* window;
		int viewportWidth;
		int viewportHeight;

		std::vector<Object> objects;
		
		std::string resourceDir = "./";
		Program bphongProg;
		glm::vec3 lightPositions[MAX_LIGHTS];
		glm::vec3 lightColors[MAX_LIGHTS];
		int lightCount;

		Camera camera;
		MatrixStack P;
		MatrixStack MV;
		
		// GLFW Callback Functions (glfw expects first arg to be window pointer even though its a member variable)
		void errorCallback(int error, const char *description);
		void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		void mouseCallback(GLFWwindow* window, int button, int action, int mods);
		void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
		void charCallback(GLFWwindow* window, unsigned int c);
		void resizeCallback(GLFWwindow* window, int width, int height);
		
		// 😻
		void render();
};