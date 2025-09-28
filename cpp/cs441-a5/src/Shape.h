#pragma once
#ifndef SHAPE_H
#define SHAPE_H

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
	Shape();
	virtual ~Shape();
	void loadMeshFile(const std::string &meshName);
	void loadMeshBuffers(std::vector<float> posBuf, std::vector<float> norBuf, std::vector<float> texBuf, std::vector<unsigned int> indBuf);
	void fitToUnitBox();
	void init();
	void draw(const std::shared_ptr<Program> prog) const;
	float getBaseY();

	static std::shared_ptr<Shape> buildSphere(int v);
	static std::shared_ptr<Shape> buildSOR(int v);
	static void pushvec(std::vector<float>& buf, float f1, float f2, float f3);
	static void pushvec(std::vector<float>& buf, float f1, float f2);
	static void pushvec(std::vector<unsigned int>& buf, unsigned int ui1, unsigned int ui2, unsigned int ui3);
	
private:
	std::vector<float> posBuf;
	std::vector<float> norBuf;
	std::vector<float> texBuf;
	std::vector<unsigned int> indBuf;
	unsigned posBufID;
	unsigned norBufID;
	unsigned texBufID;
	unsigned indBufID;
	float baseY;
	bool procedural;

	void drawArrays(const std::shared_ptr<Program> prog) const;
	void drawElements(const std::shared_ptr<Program> prog) const;
};

#endif
