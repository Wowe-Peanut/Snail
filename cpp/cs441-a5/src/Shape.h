#pragma once

#include <string>
#include <vector>
#include <memory>

class Program;

/**
 * A shape defined by a list of triangles
 * - posBuf should be of length 3*ntris
 * - norBuf should be of length 3*ntris (if normals are available)
 * - texBuf should be of length 2*ntris (if texture coords are available)
 * posBufID, norBufID, and texBufID are OpenGL buffer identifiers.
 */
class Shape
{
public:
	std::vector<float> posBuf;
	std::vector<float> norBuf;
	std::vector<float> texBuf;
	std::vector<unsigned int> indBuf;
	unsigned posBufID;
	unsigned norBufID;
	unsigned texBufID;
	unsigned indBufID;

	std::vector<std::vector<unsigned int>> edgeList;
	std::vector<float> lengthsSquared; // resting distance of edges in mesh squared (for spring calculations)

	float baseY;
	bool procedural;


	Shape();
	virtual ~Shape();
	void loadMeshFile(const std::string &meshName);
	void loadMeshBuffers(std::vector<float> posBuf, std::vector<float> norBuf, std::vector<float> texBuf, std::vector<unsigned int> indBuf);
	void fitToUnitBox();
	void init();

	float getBaseY();
	void draw(const std::shared_ptr<Program> prog) const;
	void drawArrays(const std::shared_ptr<Program> prog) const;
	void drawElements(const std::shared_ptr<Program> prog) const;

	static std::shared_ptr<Shape> buildSphere(int v);
	static std::shared_ptr<Shape> buildCube(float segmentLength, int segments);
	static std::shared_ptr<Shape> buildPlane(float segmentLength, int segments);
};
