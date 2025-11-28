#pragma once

#include "program.h"
#include "object.h"

#include <string>
#include <vector>
#include <memory>


// Geometric Primitives: 2,3,4 vertex indices & resting qualities
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
		int numPoints;			// Number of vertices (internal AND external)
		bool isStatic;			// Determines GL_STREAM_DRAW or GL_STATIC_DRAW
		bool useIndBuf;			// Yes: drawElements, No: drawArrays

		template <typename T>
		void initBuffer(GLenum glBufferType, std::vector<T>& buffer, unsigned& bufferID);
		Mesh(std::string filePath, bool isStatic);
		void init();


		template <typename T>
		void updateBuffer(const std::shared_ptr<Program> prog, std::string attribName, std::vector<T>& buffer, unsigned bufferID, int valuesPerVertex);
		void draw(const std::shared_ptr<Program> prog);	
		void drawElements(const std::shared_ptr<Program> prog);
		void drawArrays(const std::shared_ptr<Program> prog);


		void loadMshFile(std::string mshFilePath);
		void loadObjFile(std::string objFilePath);
		void computeNormals(); 
		void transform(Transform transform);
};

 