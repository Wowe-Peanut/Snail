#include "Shape.h"
#include <filesystem>
#include <iostream>
#include <vector>
#include <memory>
#include <fstream>

#include "GLSL.h"
#include "Program.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

using namespace std;

Shape::Shape(string filePath): posBufID(0), norBufID(0), texBufID(0) {

	filesystem::path p(filePath);
	string extension = p.extension().string();

	if (extension == ".obj") {
		loadObjFile(filePath);
	} else if (extension == ".msh") {
		loadMeshFile(filePath);
	} else {
		cerr << "Unsupported mesh filetype: " << extension << endl; 
	}
}

void Shape::init() {	

	// Send the position array to the GPU
	glGenBuffers(1, &posBufID);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_DYNAMIC_DRAW);
	
	// Send the normal array to the GPU
	if(!norBuf.empty()) {
		glGenBuffers(1, &norBufID);
		glBindBuffer(GL_ARRAY_BUFFER, norBufID);
		glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_DYNAMIC_DRAW);
	}
	
	// Send the texture array to the GPU
	if(!texBuf.empty()) {
		glGenBuffers(1, &texBufID);
		glBindBuffer(GL_ARRAY_BUFFER, texBufID);
		glBufferData(GL_ARRAY_BUFFER, texBuf.size()*sizeof(float), &texBuf[0], GL_STATIC_DRAW);
	}

	// Send the index array to the GPU
	if (!indBuf.empty()) {
		glGenBuffers(1, &indBufID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indBufID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indBuf.size()*sizeof(unsigned int), &indBuf[0], GL_STATIC_DRAW);
	}
	
	// Unbind the arrays
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);	

	GLSL::checkError(GET_FILE_LINE);
}
void Shape::loadObjFile(string filePath) {

	// I'm currently not using .obj files for physics stuff so keep them 'drawArray' so I can continue to use this implementation
	drawWithElements = false;

	// Load geometry
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	string warnStr, errStr;
	bool rc = tinyobj::LoadObj(&attrib, &shapes, &materials, &warnStr, &errStr, filePath.c_str());
	if(!rc) {
		cerr << errStr << endl;
	} else {

		// Some OBJ files have different indices for vertex positions, normals,
		// and texture coordinates. For example, a cube corner vertex may have
		// three different normals. Here, we are going to duplicate all such
		// vertices.
		// Loop over shapes
		for(size_t s = 0; s < shapes.size(); s++) {
			// Loop over faces (polygons)
			size_t index_offset = 0;
			for(size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
				size_t fv = shapes[s].mesh.num_face_vertices[f];
				// Loop over vertices in the face.
				for(size_t v = 0; v < fv; v++) {
					// access to vertex
					tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
					posBuf.push_back(attrib.vertices[3*idx.vertex_index+0]);
					posBuf.push_back(attrib.vertices[3*idx.vertex_index+1]);

					posBuf.push_back(attrib.vertices[3*idx.vertex_index+2]);
					if(!attrib.normals.empty()) {
						norBuf.push_back(attrib.normals[3*idx.normal_index+0]);
						norBuf.push_back(attrib.normals[3*idx.normal_index+1]);
						norBuf.push_back(attrib.normals[3*idx.normal_index+2]);
					}
					if(!attrib.texcoords.empty()) {
						texBuf.push_back(attrib.texcoords[2*idx.texcoord_index+0]);
						texBuf.push_back(attrib.texcoords[2*idx.texcoord_index+1]);
					}
				}
				index_offset += fv;
				// per-face material (IGNORE)
				shapes[s].mesh.material_ids[f];
			}
		}
	}
}
void Shape::loadMeshFile(string filePath) {

	// Physics sims require draw by element and I'm using .msh for volumetric physics meshes, ergo drawWithElements
	drawWithElements = true;


	ifstream f(filePath);
	if (!f.is_open()) {
		cerr << "Failed to open '" << filePath << "'" << endl;
		exit(1);
	}

	// Nodes are vertices in gmsh
	string line;
	while (getline(f, line)) {
		if (line.find("$Nodes") != string::npos) {

			// Read in node metadata
			int numBlocks, totalNodes, minTag, maxTag;
			f >> numBlocks >> totalNodes >> minTag >> maxTag;

			// Initialize the position buffer
			posBuf = vector<float>(totalNodes*3);
		
			// Iterate through each node block (gmsh likes to separate separate entities into separate blocks)
			for (int block=0; block<numBlocks; block++) {
				int entityDim, entityTag, parametric, numNodesInBlock;
				f >> entityDim >> entityTag >> parametric >> numNodesInBlock;

				// Read node indices
				vector<int> nodeIdxs(numNodesInBlock);
				for (int node=0; node<numNodesInBlock; node++) {
					f >> nodeIdxs[node];
				}
				
				// Add node positions to position buffer, ordered by ID
				for (int node=0; node<numNodesInBlock; node++) {
					int nodeIdx = nodeIdxs[node] - 1;
					f >> posBuf[nodeIdx*3] >> posBuf[nodeIdx*3 + 1] >> posBuf[nodeIdx*3 + 2];
				}
			}
			break;
		}
	}
	
	// Elements connect nodes together into primitives
	while (getline(f, line)) {
		if (line.find("$Elements") != string::npos) {
			
			int numBlocks, totalElem, minTag, maxTag;
			f >> numBlocks >> totalElem >> minTag >> maxTag;

			// Iterate through each element blocks
			for (int block=0; block<numBlocks; block++) {
				int entityDim, entityTag, parametric, numElemInBlock;
				f >> entityDim >> entityTag >> parametric >> numElemInBlock;

				
				// Build in elements in this entity (e.g. triangles, tedrahedrals, etc...)
				for (int element=0; element<numElemInBlock; element++) {
					int elementID;
					f >> elementID;

					// Triangles (dim 2) have 3 nodes, Tetrahedra (dim 3) have 4 nodes
					int nodesInElement = entityDim == 2 ? 3 : 4; 
					vector<int> nodes(nodesInElement);
					for (int node=0; node<nodesInElement; node++) { 
						f >> nodes[node];
						nodes[node]--; // make zero-indexed
					}

					// Insert triangles into indBuf, insert tetrahedra into edgelist
					if (entityDim == 2) { 
						indBuf.insert(indBuf.end(), nodes.begin(), nodes.end());	
					} else if (entityDim == 3) { 
						for (size_t n1=0; n1<nodes.size(); n1++) {
							for (size_t n2=n1+1; n2<nodes.size(); n2++) {
								
								// Sort by index so it's easier to remove duplicates later
								if (nodes[n1] < nodes[n2]) 	edgeList.push_back({nodes[n1], nodes[n2]});
								else 						edgeList.push_back({nodes[n2], nodes[n1]});
							}
						}
					}

					
				}
				
			}

			break;
		}
	}

	// Remove duplicate edges (gmsh tends to overdue it... 😿)
	set<vector<int>> uniqueEdges(edgeList.begin(), edgeList.end());
	edgeList = vector<vector<int>>(uniqueEdges.begin(), uniqueEdges.end());
	

	// Calculate resting edge lengths
	for (size_t edgeIdx=0; edgeIdx<edgeList.size(); edgeIdx++) {
		int vidx1 = edgeList[edgeIdx][0];
		int vidx2 = edgeList[edgeIdx][1];
		edgeRestLengthSquares.push_back(pow(posBuf[3*vidx1] - posBuf[3*vidx2], 2) + pow(posBuf[3*vidx1+1] - posBuf[3*vidx2+1], 2) + pow(posBuf[3*vidx1+2] - posBuf[3*vidx2+2], 2));
	}

	// TODO - normal buffer initialization
	// TODO - texture buffer initialization

	f.close();
}

void Shape::draw(const shared_ptr<Program> prog) const {
	if (drawWithElements) {
		drawElements(prog);
	} else {
		drawArrays(prog);
	}
}
void Shape::drawElements(const shared_ptr<Program> prog) const {
	int aPos = prog->getAttribute("aPos");
	if (aPos != -1 && posBufID != 0) {
    	glEnableVertexAttribArray(prog->getAttribute("aPos"));
		glBindBuffer(GL_ARRAY_BUFFER, posBufID);
		glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_DYNAMIC_DRAW);
		glVertexAttribPointer(prog->getAttribute("aPos"), 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
	}

	int aNor = prog->getAttribute("aNor");
	if (aNor != -1 && posBufID != 0) {
		glEnableVertexAttribArray(prog->getAttribute("aNor"));
		glBindBuffer(GL_ARRAY_BUFFER, norBufID);
		glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_DYNAMIC_DRAW);
		glVertexAttribPointer(prog->getAttribute("aNor"), 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
	}	
		
    glBindBuffer(GL_ARRAY_BUFFER, texBufID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indBufID);

    glDrawElements(GL_TRIANGLES, indBuf.size(), GL_UNSIGNED_INT, (void *)0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (aNor != -1) glDisableVertexAttribArray(aNor);
    if (aPos != -1) glDisableVertexAttribArray(aPos);	

	GLSL::checkError(GET_FILE_LINE);
}
void Shape::drawArrays(const shared_ptr<Program> prog) const {
	// Bind position buffer
	int h_pos = prog->getAttribute("aPos");
	glEnableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	
	// Bind normal buffer
	int h_nor = prog->getAttribute("aNor");
	if(h_nor != -1 && norBufID != 0) {
		glEnableVertexAttribArray(h_nor);
		glBindBuffer(GL_ARRAY_BUFFER, norBufID);
		glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	}
	
	// Bind texcoords buffer
	int h_tex = prog->getAttribute("aTex");
	if(h_tex != -1 && texBufID != 0) {
		glEnableVertexAttribArray(h_tex);
		glBindBuffer(GL_ARRAY_BUFFER, texBufID);
		glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	}
	
	// Draw
	int count = posBuf.size()/3; // number of indices to be rendered
	glDrawArrays(GL_TRIANGLES, 0, count);
	
	// Disable and unbind
	if(h_tex != -1) {
		glDisableVertexAttribArray(h_tex);
	}
	if(h_nor != -1) {
		glDisableVertexAttribArray(h_nor);
	}
	glDisableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	GLSL::checkError(GET_FILE_LINE);
}
