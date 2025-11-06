#pragma once

#include <cassert>
#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
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

#define MAX_LIGHTS 10
#define DEFAULT_WIDTH 800;
#define DEFAULT_HEIGHT 600;

/** 
 * My goal is to be able to call like 
 * './A5 ../resources -r replay.txt' and have it replay the animation or
 * or something like
 * './A5 ../resources -s setup.json
 * 
 * It should be able to:
 * 		Replay 
 * 		Simulate (with options to 'render & save', 'save', 'render', 'save then render')
 * 			'save then render' should pretty much be running replay mode right after
 * 			'saving' should be a world-state-frames not image-frames (that way you can still move camera around in replay mode)
 * 				and it should only save visibile triangles positions & normals (it don't care about internal shit)
 * 			Saving will require setting a range of time to simulate 
 * 		
 * 		If replaying or simulating with 'render' you should be able to interact with animation:
 * 			- Toggle pausing, culling, single step forward, reseting
 * 			- Camera should be able to rotate around center, zoom in/out
 * 
 * When we are simulating but only 'saving' and not 'rendering', it shouldn't interact with the opengl shit at all
 * for speed and separation purposes. That means I need a way to easily link the two systems so that is quite efficient 
 * while running in the case where I want to do both.
 * 
 * So I think that there should be a PhysicsEngine class that can be initialized with all of the appropriate 
 * constants (h, tol, maxiter, etc) that belongs to the Renderer who uses it on the objects if in 'simulate' mode.
 * These parameter should be set in the json file input file. The only concern I have now is with interactions between
 * objects. It seems like the PhysicsEngine will need to be passed references to all the objects and initialized by being given
 * which objects are allowed to collide and how (how to tell it is tbh).
 * 		This way, the renderer need but call .stepAllObjects() on the engine and it will update the objects and then it can draw everything
 * 
 * For that to work with some of the other things I have in mind, I need to modify the object & shape class to keep visibile and hidden
 * parts of their mesh separate. Since renderer should only be drawing the external triangles and the object needs a way to take
 * the values it gets from the physics simulation and copy just the outer triangles to the renderes buffer. But for now I'll keep 
 * the renderer drawing everything as it is and change the object class later
 */

class Renderer {
	public:

		enum class ViewingToggles {
			CULLING 			= 1 << 0, // Doesn't draw backwards facing triangles
			FILL_TRIANGLES 		= 1 << 1,
			PAUSED				= 1 << 2,
		};

		Renderer();
		~Renderer();

		void init();
		void render();
		void playback();
		void simulate();

	private:

		GLFWwindow* window; // Pointer b/c that is what GLFW expects
		Camera camera;
		int viewportWidth;
		int viewportHeight;
		std::string resouorceDir = "./";
		unsigned int toggles;
		
		Program bphongProg;
		std::vector<Object> objects;
		glm::vec3 lightPositions[MAX_LIGHTS];
		glm::vec3 lightColors[MAX_LIGHTS];
		int lightCount;

		MatrixStack P;
		MatrixStack MV;
		

		
		// GLFW Callback Functions (glfw expects first arg to be the window even though its a member variable)
		void errorCallback(int error, const char *description);
		void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		void mouseCallback(GLFWwindow* window, int button, int action, int mods);
		void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
		void charCallback(GLFWwindow* window, unsigned int c);
		void resizeCallback(GLFWwindow* window, int width, int height);
};