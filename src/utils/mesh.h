#pragma once

#include "program.h"
#include "json.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>
#include <Eigen/Dense>

struct AABB {
	glm::vec3 mins;
	glm::vec3 maxs;

	// Triangle constructor
	AABB(const Eigen::Vector3d& v1, const Eigen::Vector3d& v2, const Eigen::Vector3d& v3) {
		mins = {min(v1.x, min(v2.x, v3.x)), min(v1.y, min(v2.y, v3.y)), min(v1.z, min(v2.z, v3.z))};
		maxs = {max(v1.x, max(v2.x, v3.x)), max(v1.y, min(v2.y, v3.y)), max(v1.z, max(v2.z, v3.z))};
	}

	bool overlaps(const AABB& other) {
		return 	(mins.x <= other.maxs.x && maxs.x >= other.mins.x) &&
         		(mins.y <= other.maxs.y && maxs.y >= other.mins.y) &&
         		(mins.z <= other.maxs.z && maxs.z >= other.mins.z);
	}
};

struct TriangleBBComparatorX {
	bool operator()(const Triangle& a, const Triangle& b) const {
		return a.boundingBox.mins.x < b.boundingBox.mins.x;
	}
};

struct Transform {
	glm::vec3 translation;
	glm::vec3 rotation;
	glm::vec3 scale;
};

struct Edge {
	int v1, v2;
	double l2;

    bool operator<(const Edge& other) const {
        if (v1 != other.v1) return v1 < other.v1;
		else 				return v2 < other.v2;
    }
};

struct Triangle {
	int v1, v2, v3;
	AABB boundingBox;
};

struct Tetrahedron {
	int v1, v2, v3, v4;
};

 
struct Mesh {

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
	glm::vec3 initialVelocity;

	template <typename T>
	void initBuffer(GLenum glBufferType, std::vector<T>& buffer, unsigned& bufferID);
	Mesh(std::string filePath, bool isStatic, glm::vec3 preInitScale, Transform meshTransform, std::vector<int>& fixedPoints, glm::vec3 velocity);
	void init();

	void updateBuffer(const std::shared_ptr<Program> prog, GLint attribID, std::vector<float>& buffer, unsigned bufferID, int valuesPerVertex);
	void draw(const std::shared_ptr<Program> prog);	

	void computeRestingEdgeLengths();
	void computeSurfaceQualities();
	void transform(Transform transform);
	void setFixedPoints(std::vector<int>& fixedPoints);
	void loadMshFile(std::string mshFilePath);
};

 