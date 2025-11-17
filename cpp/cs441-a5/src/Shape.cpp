#include "Shape.h"
#include <algorithm>
#include <iostream>
#include <vector>
#include <memory>

#include "GLSL.h"
#include "Program.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

using namespace std;

Shape::Shape(): posBufID(0), norBufID(0), texBufID(0), procedural(false) {}
Shape::~Shape() {}
float Shape::getBaseY() { return this->baseY; }

shared_ptr<Shape> Shape::buildPlane(float segmentLength, int segments) {
	shared_ptr<Shape> plane = make_shared<Shape>();

	for (int x = 0; x <= segments; x++) {
		for (int y = 0; y <= segments; y++) {
			plane->posBuf.insert(plane->posBuf.end(), {x*segmentLength, 0, y*segmentLength});
			plane->norBuf.insert(plane->norBuf.end(), {0,1,0}); // Upward normals
			

			// Construct triangles
			unsigned int k = x + (segments+1)*y;

			if (x < segments && y < segments) {
				unsigned int k = x + (segments+1)*y;
				plane->indBuf.insert(plane->indBuf.end(), {k, k+1, k+segments+2, k+segments+2, k+segments+1, k});
			}

			// Construct spring edges
			if (x < segments) {	// Horizontal
				plane->edgeList.push_back({k, k+1});
			}
			if (y < segments) { // Vertical
				plane->edgeList.push_back({k, k+segments+1});
			}
			if (x < segments && y < segments) { // Diagonal
				plane->edgeList.push_back({k, k+segments+2});
				plane->edgeList.push_back({k+1, k+segments+1});
			}
		}
	}



	
	// Calculate resting spring lengths squared
	for (int edgeIdx=0; edgeIdx < plane->edgeList.size(); edgeIdx++) {
		int vIdx1 = plane->edgeList[edgeIdx][0];
		int vIdx2 = plane->edgeList[edgeIdx][1];
		plane->lengthsSquared.push_back(pow(plane->posBuf[3*vIdx1] - plane->posBuf[3*vIdx2], 2) + pow(plane->posBuf[3*vIdx1+1] - plane->posBuf[3*vIdx2+1], 2) + pow(plane->posBuf[3*vIdx1+2] - plane->posBuf[3*vIdx2+2], 2));
	}

	plane->procedural = true;
	plane->init();
	return plane;
}

shared_ptr<Shape> Shape::buildCube(float segmentLength, int segments) {
	shared_ptr<Shape> cube = make_shared<Shape>();
	
	// References to cube buffers to make the construction below more readable
	vector<float>& posBuf = cube->posBuf;
	vector<float>& norBuf = cube->norBuf;
	vector<float>& texBuf = cube->texBuf;
	vector<unsigned int>& indBuf = cube->indBuf;
	vector<vector<unsigned int>>& edgeList = cube->edgeList;
	vector<float>& lengthsSquared = cube->lengthsSquared;

	// Sample uniformly in cube from (0,0,0) -> (sideLength, sideLength, sideLength)
	for (int x = 0; x <= segments; x++) {
		for (int y = 0; y <= segments; y++) {
			for (int z = 0; z <= segments; z++) {

				posBuf.insert(posBuf.end(), {x*segmentLength, y*segmentLength, z*segmentLength});
				

				norBuf.insert(norBuf.end(), {0,0,0}); // Placeholder normals
				texBuf.insert(texBuf.end(), {0,0}); // Placeholder tex coords
				
				// If not on the far faces, construct the unit cube 
				if (x < segments && y < segments && z < segments) {
					
					// Calculate positions indices of the unit cube
					vector<unsigned int> idxs;
					for (int dx=0; dx<=1; dx++) {
						for (int dy=0; dy<=1; dy++) {
							for (int dz=0; dz<=1; dz++) {
								idxs.push_back((x+dx)*(segments+1)*(segments+1) + (y+dy)*(segments+1) + (z+dz));
							}
						}	
					}
					
					// Construct edges (all pairs of unit cube indices, no duplicates, order doesn't matter) and calculate resting spring length squared
					for (int i=1; i<idxs.size(); i++) {
						for (int j=0; j<i; j++) {
							edgeList.push_back({idxs[j], idxs[i]});
						}
					}
					
					// Construct triangles (currently all of them)
					for (int i=2; i<idxs.size(); i++) {
						for (int j=1; j<i; j++) {
							for (int k=0; k<j; k++) {
								vector<unsigned int> face = {idxs[i], idxs[j], idxs[k]};
								
								// TODO: get it to only draw surface triangles
								indBuf.insert(indBuf.end(), face.begin(), face.end());
							}
						}
					}
				}

			}	
		}
	}

	// Calculate resting spring lengths squared
	for (int edgeIdx=0; edgeIdx<edgeList.size(); edgeIdx++) {
		int vIdx1 = edgeList[edgeIdx][0];
		int vIdx2 = edgeList[edgeIdx][1];
		lengthsSquared.push_back(pow(posBuf[3*vIdx1] - posBuf[3*vIdx2], 2) + pow(posBuf[3*vIdx1+1] - posBuf[3*vIdx2+1], 2) + pow(posBuf[3*vIdx1+2] - posBuf[3*vIdx2+2], 2));
	}
	

	cube->procedural = true;
	cube->init();
	return cube;
}


/**
 * So after doing some research I've come face-to-face with the world of volumetric mesh generation. Needless to say it's fucking massive and super cool. However, for now I'm going 
 * to focus on the simulating part and perhaps I can revisit that later on. Instead, I'm going to look into some external libraries to generate volumetric meshes and convert
 * them to a format that this fucking shit show of an engine can understand and work with. The main difficulty will be my original goal with this improvement, which was not sending
 * internal triangles to the GPU, however this also has the benefit of allowing for any models to be imported.
 * 
 * 
 */


void Shape::loadMeshFile(const string &meshName)
{
	this->procedural = false;

	// Load geometry
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	string warnStr, errStr;
	bool rc = tinyobj::LoadObj(&attrib, &shapes, &materials, &warnStr, &errStr, meshName.c_str());
	if(!rc) {
		cerr << errStr << endl;
	} else {

		this->baseY = FLT_MAX;

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

					baseY = min(baseY, posBuf.back());	

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

void Shape::fitToUnitBox()
{
	// Scale the vertex positions so that they fit within [-1, +1] in all three dimensions.
	glm::vec3 vmin(posBuf[0], posBuf[1], posBuf[2]);
	glm::vec3 vmax(posBuf[0], posBuf[1], posBuf[2]);
	for(int i = 0; i < (int)posBuf.size(); i += 3) {
		glm::vec3 v(posBuf[i], posBuf[i+1], posBuf[i+2]);
		vmin.x = min(vmin.x, v.x);
		vmin.y = min(vmin.y, v.y);
		vmin.z = min(vmin.z, v.z);
		vmax.x = max(vmax.x, v.x);
		vmax.y = max(vmax.y, v.y);
		vmax.z = max(vmax.z, v.z);
	}
	glm::vec3 center = 0.5f*(vmin + vmax);
	glm::vec3 diff = vmax - vmin;
	float diffmax = diff.x;
	diffmax = max(diffmax, diff.y);
	diffmax = max(diffmax, diff.z);
	float scale = 1.0f / diffmax;
	for(int i = 0; i < (int)posBuf.size(); i += 3) {
		posBuf[i  ] = (posBuf[i  ] - center.x) * scale;
		posBuf[i+1] = (posBuf[i+1] - center.y) * scale;
		posBuf[i+2] = (posBuf[i+2] - center.z) * scale;
	}
}

void Shape::init()
{	
	// Find base height of object
	this->baseY = FLT_MAX;
	for (size_t yi = 1; yi<posBuf.size(); yi+=3) {
		this->baseY = min(this->baseY, posBuf[yi]);
	}

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

void Shape::draw(const shared_ptr<Program> prog) const {
	if (this->procedural) 	this->drawElements(prog);
	else					this->drawArrays(prog);
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
