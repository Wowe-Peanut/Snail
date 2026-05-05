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
	Eigen::Vector3d mins;
	Eigen::Vector3d maxs;

	// Triangle constructor
	AABB(): mins(0.0, 0.0, 0.0), maxs(0.0, 0.0, 0.0) {};
	AABB(const Eigen::Vector3d& v1, const Eigen::Vector3d& v2, const Eigen::Vector3d& v3);
	bool overlaps(AABB& other);
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

	Triangle(int v1, int v2, int v3): v1(v1), v2(v2), v3(v3) {};
};

struct Tet {
	int v1, v2, v3, v4;
	
	// Inverse init transform, precomputed for faster deformation gradient calculations
	Eigen::Matrix3d B; 
	
	// Precomputed b/c they only depend on B 
	// Uses 3d tensor format from chapter 3 of: https://www.tkim.graphics/DYNAMIC_DEFORMABLES/DynamicDeformables.pdf
	std::vector<Eigen::Matrix3d> dfdv1 = {Eigen::Matrix3d::Zero(), Eigen::Matrix3d::Zero(), Eigen::Matrix3d::Zero()};
	std::vector<Eigen::Matrix3d> dfdv2 = {Eigen::Matrix3d::Zero(), Eigen::Matrix3d::Zero(), Eigen::Matrix3d::Zero()};
	std::vector<Eigen::Matrix3d> dfdv3 = {Eigen::Matrix3d::Zero(), Eigen::Matrix3d::Zero(), Eigen::Matrix3d::Zero()};
	std::vector<Eigen::Matrix3d> dfdv4 = {Eigen::Matrix3d::Zero(), Eigen::Matrix3d::Zero(), Eigen::Matrix3d::Zero()};

	
	void init(Eigen::Vector3d p1, Eigen::Vector3d p2, Eigen::Vector3d p3, Eigen::Vector3d p4);
	Eigen::Matrix3d F(Eigen::Vector3d p1, Eigen::Vector3d p2, Eigen::Vector3d p3, Eigen::Vector3d p4);
};

struct TriangleBBComparatorX {
	bool operator()(const Triangle& a, const Triangle& b) const {
		return a.boundingBox.mins.x() < b.boundingBox.mins.x();
	}
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
	std::vector<Tet> tets;

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

	void computeRestingEdges();
	void computeSurfaceQualities();
	void computeRestingTets();

	void transform(Transform transform);
	void setFixedPoints(std::vector<int>& fixedPoints);
	void loadMshFile(std::string mshFilePath);
};

 