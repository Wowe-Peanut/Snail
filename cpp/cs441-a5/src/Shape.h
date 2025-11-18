#pragma once

#include <string>
#include <vector>
#include <memory>

class Program;

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

		// Spring edges
		std::vector<std::vector<unsigned int>> edgeList;
		std::vector<float> edgeRestLengthSquares;
		
		Shape();
		void init(); // Initializes OpenGL buffers

		void loadFromFile(const std::string &fileName);
		void loadObjFile(const std::string &fileName);	// .obj
		void loadMeshFile(const std::string &fileName);	// .msh

		bool drawWithElements;
		void draw(const std::shared_ptr<Program> prog) const;			
		void drawArrays(const std::shared_ptr<Program> prog) const;		// Triangles defined by structure of 'posBuf'
		void drawElements(const std::shared_ptr<Program> prog) const;	// Triangles defined by 'posBuf' + 'indBuf'

};
