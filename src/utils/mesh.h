#pragma once

#include "program.h"

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

		int numPoints;
		
		// Main
		Mesh();
		void init(GLenum glBufferType);
		virtual void draw(const std::shared_ptr<Program> prog) = 0;	

		// Helper
		template <typename T>
		void initBuffer(GLenum glBufferType, std::vector<T>& buffer, unsigned& bufferID);
};

// .msh files, draws with element
class DynamicMesh : public Mesh {
	public:
		// Surface = 2 dim, Volume = 3 dim
		int dim; 
		std::vector<Edge> edges;
		std::vector<Triangle> triangles;
		std::vector<Tetrahedron> tetrahedron;

		DynamicMesh(int dimension, std::string mshFilePath);

		void loadMeshFile(std::string mshFilePath);
		void draw(const std::shared_ptr<Program> prog);
		void computeNormals();
		void init();
		void updateBuffer(const std::shared_ptr<Program> prog, std::string attribName, std::vector<float>& buffer, unsigned bufferID, int valuesPerVertex);
};


// .obj files, no primitives, draws with arrays
class StaticMesh: public Mesh {
	public:
		StaticMesh(std::string objFilePath);

		void loadObjFile(std::string objFilePath);	
		void draw(const std::shared_ptr<Program> prog);
		void init();
};