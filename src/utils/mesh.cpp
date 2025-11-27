
#include "mesh.h"
#include "glsl.h"
#include "program.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <filesystem>
#include <iostream>
#include <vector>
#include <memory>
#include <set>
#include <fstream>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

using namespace std;
using vec3 = glm::vec3;

// External helpers
vec3 bufToVec(vector<float>& buffer, int idx) {
	return vec3(buffer[idx*3], buffer[idx*3 + 1], buffer[idx*3 + 2]);
}

void vecToBuf(vector<float>& buffer, vec3& vec, int idx) {
	buffer[idx*3] = vec[0];
	buffer[idx*3 + 1] = vec[1];
	buffer[idx*3 + 2] = vec[2];
}

string getExtension(string path) {
	filesystem::path p(path);
	return p.extension().string();
}


// Parent Mesh
// -----------------------------------------------------------------------------
Mesh::Mesh(): triPosBufID(0), triNorBufID(0), triTexBufID(0) {}

void Mesh::init(GLenum glBufferType) {

	// Initialize position/normal buffers with 'bufferType' in [GL_STATIC_DRAW, GL_STREAM_DRAW]
	// and texture/index buffers with GL_STATIC_DRAW
	initBuffer(glBufferType, triPosBuf, triPosBufID);
	initBuffer(glBufferType, triNorBuf, triNorBufID);
	initBuffer(GL_STATIC_DRAW, triIndBuf, triTexBufID);
	initBuffer(GL_STATIC_DRAW, triIndBuf, triIndBufID);

	// Unbind the arrays
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);	

	GLSL::checkError(GET_FILE_LINE);
}

template<typename T>
void Mesh::initBuffer(GLenum glBufferType, vector<T>& buffer, unsigned& bufferID) {
	if (!buffer.empty()) {
		glGenBuffers(1, &bufferID);
		glBindBuffer(GL_ARRAY_BUFFER, bufferID);
		glBufferData(GL_ARRAY_BUFFER, buffer.size()*sizeof(T), &buffer[0], glBufferType);
	}
}



// Dynamic Mesh
// -----------------------------------------------------------------------------
DynamicMesh::DynamicMesh(int dimension, string mshFilePath): dim(dimension) {

	string extension = getExtension(mshFilePath);
	if (extension != ".msh") {
		cerr << "DynamicMesh expected '.msh', got '" << extension << "'. Check mesh file type silly!" << endl;
		exit(1);
	}

	loadMeshFile(mshFilePath);
}

void DynamicMesh::loadMeshFile(string mshFilePath) {

	ifstream f(mshFilePath);
	if (!f.is_open()) {
		cerr << "Failed to open '" << mshFilePath << "'" << endl;
		exit(1);
	}

	// Nodes are vertices in gmsh
	string line;
	while (getline(f, line)) {
		if (line.find("$Nodes") != string::npos) {

			// Read in node metadata
			int numBlocks, totalNodes, minTag, maxTag;
			f >> numBlocks >> totalNodes >> minTag >> maxTag;

			// Initialize the opengl buffers
			triPosBuf = vector<float>(totalNodes*3);
			triNorBuf = vector<float>(totalNodes*3);
			numPoints = totalNodes;
		
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
					f >> triPosBuf[nodeIdx*3] >> triPosBuf[nodeIdx*3 + 1] >> triPosBuf[nodeIdx*3 + 2];
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

					// Surface Primitives
					if (entityDim == 2) { 
						triIndBuf.insert(triIndBuf.end(), nodes.begin(), nodes.end());	
						triangles.push_back({nodes[0], nodes[1], nodes[2]});
					
					// Volumetric Primitives
					} else if (entityDim == 3) { 
						for (size_t n1=0; n1<nodes.size(); n1++) {
							for (size_t n2=n1+1; n2<nodes.size(); n2++) {
								
								// Sort by index so it's easier to remove duplicates later
								if (nodes[n1] < nodes[n2]) 	edges.push_back({nodes[n1], nodes[n2]});
								else 						edges.push_back({nodes[n2], nodes[n1]});
							}
						}

						tetrahedron.push_back({nodes[0], nodes[1], nodes[2], nodes[3]});
					} 
				}
				
			}

			break;
		}
	}

	// Remove duplicate edges (gmsh tends to overdue it... 😿)
	set<Edge> uniqueEdges(edges.begin(), edges.end());
	edges = vector<Edge>(uniqueEdges.begin(), uniqueEdges.end());
	
	// Calculate resting edge lengths
	for (Edge& edge: edges) {
		float dx2 = pow(triPosBuf[3*edge.v1] - triPosBuf[3*edge.v2], 2);
		float dy2 = pow(triPosBuf[3*edge.v1+1] - triPosBuf[3*edge.v2+1], 2);
		float dz2 = pow(triPosBuf[3*edge.v1+2] - triPosBuf[3*edge.v2+2], 2);
		
		edge.l2 = dx2 + dy2 + dz2;
	}

	// Calculate vertex normals
	computeNormals();

	// TODO - texture buffer initialization

	f.close();
}

void DynamicMesh::init() {
	Mesh::init(GL_STREAM_DRAW);
}

void DynamicMesh::draw(const shared_ptr<Program> prog) {

	// Update the three dynamic buffers
	updateBuffer(prog, "aPos", triPosBuf, triPosBufID, 3);
	updateBuffer(prog, "aNor", triNorBuf, triNorBufID, 3);
	updateBuffer(prog, "aTex", triTexBuf, triTexBufID, 2);

	// Draw triangles
    glBindBuffer(GL_ARRAY_BUFFER, triTexBufID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triIndBufID);
    glDrawElements(GL_TRIANGLES, triIndBuf.size(), GL_UNSIGNED_INT, (void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	GLSL::checkError(GET_FILE_LINE);
}

void DynamicMesh::updateBuffer(const shared_ptr<Program> prog, string attribName, vector<float>& buffer, unsigned bufferID, int valuesPerVertex) {
	int attribID = prog->getAttribute(attribName);
	if (attribID != -1 && bufferID != -1) {

		// Enable & bind
		glEnableVertexAttribArray(attribID);
		glBindBuffer(GL_ARRAY_BUFFER, bufferID);

		// Uses glBufferSubData instead of persistent map (for now) to avoid having to deal with CPU-GPU synchronization 
		glBufferSubData(GL_ARRAY_BUFFER, 0, triNorBuf.size()*sizeof(float), &triNorBuf[0]);

		// Disable & unbind
		glDisableVertexAttribArray(attribID);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

void DynamicMesh::computeNormals() {
	int numPoints = triPosBuf.size()/3;

	vector<vec3> normals;
	vector<int> vertexDegrees(numPoints, 0);

	// Calculate triangle norms
	for (Triangle& tri: triangles) {
		vec3 v1 = bufToVec(triPosBuf, tri.v1);
		vec3 v2 = bufToVec(triPosBuf, tri.v2);
		vec3 v3 = bufToVec(triPosBuf, tri.v3);	

		vec3 triNormal = glm::normalize(glm::cross(v2-v1, v3-v1));

		for (int vidx: {tri.v1, tri.v2, tri.v3}) {
			normals[vidx] += triNormal;
			vertexDegrees[vidx]++;
		}
	}

	// Vertex normals calculated as average the triangle norms of all triangles it participates in
	for (int vidx=0; vidx<numPoints; vidx++) {
		vec3 normal = glm::normalize(normals[vidx] / (float) vertexDegrees[vidx]);
		vecToBuf(triNorBuf, normal, vidx);
	}
}



// Static Mesh
// -----------------------------------------------------------------------------

StaticMesh::StaticMesh(string objFilePath) {

	string extension = getExtension(objFilePath);
	if (extension != ".obj") {
		cerr << "StaticMesh expected '.obj', got '" << extension << "'. Check mesh file type silly!" << endl;
		exit(1);
	}

	loadObjFile(objFilePath);
}

void StaticMesh::loadObjFile(string filePath) {

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
					triPosBuf.push_back(attrib.vertices[3*idx.vertex_index+0]);
					triPosBuf.push_back(attrib.vertices[3*idx.vertex_index+1]);

					triPosBuf.push_back(attrib.vertices[3*idx.vertex_index+2]);
					if(!attrib.normals.empty()) {
						triNorBuf.push_back(attrib.normals[3*idx.normal_index+0]);
						triNorBuf.push_back(attrib.normals[3*idx.normal_index+1]);
						triNorBuf.push_back(attrib.normals[3*idx.normal_index+2]);
					}
					if(!attrib.texcoords.empty()) {
						triTexBuf.push_back(attrib.texcoords[2*idx.texcoord_index+0]);
						triTexBuf.push_back(attrib.texcoords[2*idx.texcoord_index+1]);
					}
				}
				index_offset += fv;
				// per-face material (IGNORE)
				shapes[s].mesh.material_ids[f];
			}
		}
	}

	numPoints = triPosBuf.size()/3;
}

void StaticMesh::init() {
	Mesh::init(GL_STATIC_DRAW);
}

void StaticMesh::draw(const shared_ptr<Program> prog) {
	
	// Bind position buffer
	int h_pos = prog->getAttribute("aPos");
	glEnableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, triPosBufID);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	
	// Bind normal buffer
	int h_nor = prog->getAttribute("aNor");
	if(h_nor != -1 && triNorBufID != 0) {
		glEnableVertexAttribArray(h_nor);
		glBindBuffer(GL_ARRAY_BUFFER, triNorBufID);
		glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	}
	
	// Bind texcoords buffer
	int h_tex = prog->getAttribute("aTex");
	if(h_tex != -1 && triTexBufID != 0) {
		glEnableVertexAttribArray(h_tex);
		glBindBuffer(GL_ARRAY_BUFFER, triTexBufID);
		glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	}
	
	// Draw
	int count = triPosBuf.size()/3; // number of indices to be rendered
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

