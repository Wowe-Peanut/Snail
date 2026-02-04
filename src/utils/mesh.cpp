
#define TINYOBJLOADER_IMPLEMENTATION
#define GLM_FORCE_RADIANS

#include "mesh.h"
#include "glsl.h"
#include "program.h"
#include "object.h"
#include "matrix_stack.h"
#include "tiny_obj_loader.h"

#include <filesystem>
#include <iostream>
#include <vector>
#include <memory>
#include <set>
#include <map>
#include <fstream>
#include <glm/glm.hpp>
#include <Eigen/Dense>

using namespace std;
using vec3 = glm::vec3;

// Geometry methods
// ------------------------------------------------------------------------------------
AABB::AABB(const Eigen::Vector3d& v1, const Eigen::Vector3d& v2, const Eigen::Vector3d& v3) {
	mins = {min(v1.x(), min(v2.x(), v3.x())), min(v1.y(), min(v2.y(), v3.y())), min(v1.z(), min(v2.z(), v3.z()))};
	maxs = {max(v1.x(), max(v2.x(), v3.x())), max(v1.y(), min(v2.y(), v3.y())), max(v1.z(), max(v2.z(), v3.z()))};
}

bool AABB::overlaps(AABB& other) {
	return 	(mins.x() <= other.maxs.x() && maxs.x() >= other.mins.x()) &&
			(mins.y() <= other.maxs.y() && maxs.y() >= other.mins.y()) &&
			(mins.z() <= other.maxs.z() && maxs.z() >= other.mins.z());
}


// Helpers
// ------------------------------------------------------------------------------------
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



// Initialization
// ------------------------------------------------------------------------------------
Mesh::Mesh(string filePath, bool isStatic, vec3 preInitScale, Transform meshTransform, vector<int>& fixedPoints, vec3 velocity): 
triPosBufID(0), triNorBufID(0), triTexBufID(0), triIndBufID(0), isStatic(isStatic), initialVelocity(velocity) {
	string extension = getExtension(filePath);
	if (extension == ".msh") {
		loadMshFile(filePath);
		setFixedPoints(fixedPoints);

		transform({{0, 0, 0}, {0, 0, 0}, preInitScale});
		computeRestingEdgeLengths();
		transform(meshTransform);
	} else {
		cerr << "'" << extension << "' is not a supported mesh file type (.msh only atm)" << endl;
		exit(1);
	} 	
}

void Mesh::init() {
	initBuffer(isStatic ? GL_STATIC_DRAW : GL_STREAM_DRAW, triPosBuf, triPosBufID);
	initBuffer(isStatic ? GL_STATIC_DRAW : GL_STREAM_DRAW, triNorBuf, triNorBufID);
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



// Construction
// ------------------------------------------------------------------------------------
void Mesh::loadMshFile(string mshFilePath) {

	ifstream f(mshFilePath);
	if (!f.is_open()) {
		cerr << "Failed to open '" << mshFilePath << "'" << endl;
		exit(1);
	}

	// Nodes are vertices in gmsh
	string line;
	map<int, int> nodeToIndex;
	int curNodeIndex = 0;

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
				vector<int> blockNodes(numNodesInBlock);
				for (int n=0; n<numNodesInBlock; n++) {
					f >> blockNodes[n];
					nodeToIndex[blockNodes[n]] = curNodeIndex;
					curNodeIndex++;
				}
				
				// Add node positions to position buffer, ordered by ID
				for (int n=0; n<numNodesInBlock; n++) {
					int index = nodeToIndex[blockNodes[n]];
					f >> triPosBuf[index*3] >> triPosBuf[index*3 + 1] >> triPosBuf[index*3 + 2];
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

					int node;
					for (int n=0; n<nodesInElement; n++) { 
						f >> node;
						nodes[n] = nodeToIndex[node];
					}

					// Contruct edges from pairs of nodes
					for (size_t n1=0; n1<nodes.size(); n1++) {
						for (size_t n2=n1+1; n2<nodes.size(); n2++) {
							
							// Sort by index so it's easier to remove duplicates later
							if (nodes[n1] < nodes[n2]) 	edges.push_back({nodes[n1], nodes[n2]});
							else 						edges.push_back({nodes[n2], nodes[n1]});
						}
					}

					// Surface Primitives
					if (entityDim == 2) { 
						triIndBuf.insert(triIndBuf.end(), nodes.begin(), nodes.end());	
						triangles.push_back({nodes[0], nodes[1], nodes[2]});
					
					// Volumetric Primitives
					} else if (entityDim == 3) { 
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
}

void Mesh::computeRestingEdgeLengths() {
	for (Edge& edge: edges) {
		float dx2 = pow(triPosBuf[3*edge.v1] - triPosBuf[3*edge.v2], 2);
		float dy2 = pow(triPosBuf[3*edge.v1+1] - triPosBuf[3*edge.v2+1], 2);
		float dz2 = pow(triPosBuf[3*edge.v1+2] - triPosBuf[3*edge.v2+2], 2);
		
		edge.l2 = dx2 + dy2 + dz2;
	}
}

void Mesh::computeSurfaceQualities() {

	vector<vec3> normals(numPoints, vec3(0.0f));
	vector<int> vertexDegrees(numPoints, 0);

	// Calculate triangle norms
	for (Triangle& tri: triangles) {
		vec3 v1 = bufToVec(triPosBuf, tri.v1);
		vec3 v2 = bufToVec(triPosBuf, tri.v2);
		vec3 v3 = bufToVec(triPosBuf, tri.v3);	
		vec3 cr = glm::cross(v2-v1, v3-v1);
		vec3 triNormal = glm::normalize(cr);

		for (int vidx: {tri.v1, tri.v2, tri.v3}) {
			normals[vidx] += triNormal;
			vertexDegrees[vidx]++;
		}
	}

	// Each vertex norm/area is the average norm/area of all participating triangles
	for (int vidx=0; vidx<numPoints; vidx++) {
		vec3 normal = glm::normalize(normals[vidx] / (float) vertexDegrees[vidx]);
		vecToBuf(triNorBuf, normal, vidx);
	}
}

void Mesh::transform(Transform transform) {

	// Construct 4x4 transform matrix
	MatrixStack ms;
	ms.loadIdentity();
	ms.translate(transform.translation);
	ms.rotate(transform.rotation);
	ms.scale(transform.scale);
	glm::mat4 tf = ms.topMatrix();

	// Apply to each vertex
	for (int vidx=0; vidx<numPoints; vidx++) {
		glm::vec3 pos = glm::vec3(tf * glm::vec4(bufToVec(triPosBuf, vidx), 1));
		vecToBuf(triPosBuf, pos, vidx);
	}

	computeSurfaceQualities();
}

void Mesh::setFixedPoints(vector<int>& fixedPoints) {
	if (isStatic) {
		isFixedPoint = vector<bool>(numPoints, true);
		return;
	}

	isFixedPoint = vector<bool>(numPoints, false);
	for (int idx: fixedPoints) {
		isFixedPoint[idx-1] = true;
	}
}


// Drawing
// ------------------------------------------------------------------------------------
void Mesh::updateBuffer(const shared_ptr<Program> prog, GLint attribID, vector<float>& buffer, unsigned bufferID, int valuesPerVertex) {
	if (attribID != -1 && bufferID != 0) {

		// Enable & bind
		glEnableVertexAttribArray(attribID);
		glBindBuffer(GL_ARRAY_BUFFER, bufferID);

		// Uses glBufferSubData instead of persistent map (for now) to avoid having to deal with CPU-GPU synchronization 
		if (!isStatic) glBufferSubData(GL_ARRAY_BUFFER, 0, buffer.size()*sizeof(float), &buffer[0]);
		glVertexAttribPointer(attribID, valuesPerVertex, GL_FLOAT, GL_FALSE, 0, (void *)0);

		// Disable & unbind
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

// Currently only drawElements is supported
void Mesh::draw(const shared_ptr<Program> prog) {

	// Get attribute ids
	GLint aPos = prog->getAttribute("aPos");
	GLint aNor = prog->getAttribute("aNor");
	GLint aTex = prog->getAttribute("aTex");

	// Bind attribute pointers to their respective buffer (updating the buffer if not static)
	updateBuffer(prog, aPos, triPosBuf, triPosBufID, 3);
	updateBuffer(prog, aNor, triNorBuf, triNorBufID, 3);
	updateBuffer(prog, aTex, triTexBuf, triTexBufID, 2);

	// Draw triangles
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triIndBufID);
    glDrawElements(GL_TRIANGLES, triIndBuf.size(), GL_UNSIGNED_INT, (void *)0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Unbind attirbute pointers
	if (aPos != -1) glDisableVertexAttribArray(aPos);
	if (aNor != -1) glDisableVertexAttribArray(aNor);
	if (aTex != -1) glDisableVertexAttribArray(aTex);

	GLSL::checkError(GET_FILE_LINE);
}