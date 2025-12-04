#pragma once

#include "program.h"
#include "sdf.h"
#include "json.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>


struct Transform {
	glm::vec3 translation;
	glm::vec3 rotation;
	glm::vec3 scale;
};

struct Edge {
	int v1, v2;
	float l2;

    bool operator<(const Edge& other) const {
        if (v1 != other.v1) return v1 < other.v1;
		else 				return v2 < other.v2;
    }
};

struct Triangle {
	int v1, v2, v3;
};

struct Tetrahedron {
	int v1, v2, v3, v4;
};

 
class Mesh {
	public:

		// OpenGL rendering buffers
		std::vector<float> triPosBuf;
		std::vector<float> triNorBuf;
		std::vector<float> triTexBuf;
		std::vector<unsigned int> triIndBuf;

		// OpenGL rendering buffer ids
		unsigned triPosBufID;
		unsigned triNorBufID;
		unsigned triTexBufID;
		unsigned triIndBufID;

		// Geometric primitives
		std::vector<Edge> edges;
		std::vector<Triangle> triangles;
		std::vector<Tetrahedron> tetrahedron;

		// Properties
		int numPoints;			
		bool isStatic;
		std::vector<bool> isFixedPoint;
		std::shared_ptr<SDF> sdf;
		std::vector<float> vertexAreas;
		glm::vec3 initialVelocity;

		template <typename T>
		void initBuffer(GLenum glBufferType, std::vector<T>& buffer, unsigned& bufferID);
		Mesh(std::string filePath, bool isStatic, std::shared_ptr<SDF> sdf, Transform meshTransform, std::vector<int>& fixedPoints, glm::vec3 velocity);
		void init();

		void updateBuffer(const std::shared_ptr<Program> prog, GLint attribID, std::vector<float>& buffer, unsigned bufferID, int valuesPerVertex);
		void draw(const std::shared_ptr<Program> prog);	

		

		// Computes vertex normals and contact area of each vertex
		void computeSurfaceQualities();
		void transform(Transform transform);
		void setFixedPoints(std::vector<int>& fixedPoints);
		void loadMshFile(std::string mshFilePath);
};

 