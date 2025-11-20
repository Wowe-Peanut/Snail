#pragma once

#include <string>
#include <vector>
#include <memory>
#include "Program.h"

class Shape {
	public:

		// OpenGL Buffers
		std::vector<float> posBuf;
		unsigned posBufID;

		std::vector<float> norBuf;
		unsigned norBufID;

		std::vector<float> texBuf;
		unsigned texBufID;

		std::vector<unsigned int> indBuf;
		unsigned indBufID;

		std::vector<std::vector<int>> edgeList;
		std::vector<float> edgeRestLengthSquares;
		
		Shape(std::string filePath);
		void init(); // Initializes OpenGL buffers

		void loadObjFile(std::string filePath);	// .obj
		void loadMeshFile(std::string filePath);	// .msh

		bool drawWithElements;
		void draw(const std::shared_ptr<Program> prog) const;			
		void drawArrays(const std::shared_ptr<Program> prog) const;		// Triangles defined by structure of 'posBuf'
		void drawElements(const std::shared_ptr<Program> prog) const;	// Triangles defined by 'posBuf' + 'indBuf'

};
