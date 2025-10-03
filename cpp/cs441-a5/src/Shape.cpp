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

void Shape::pushvec(std::vector<float>& buf, float f1, float f2, float f3) {
	buf.push_back(f1);
	buf.push_back(f2);
	buf.push_back(f3);
}

void Shape::pushvec(std::vector<float>& buf, float f1, float f2) {
	buf.push_back(f1);
	buf.push_back(f2);
}

void Shape::pushvec(std::vector<unsigned int>& buf, unsigned int ui1, unsigned int ui2, unsigned int ui3) {
	buf.push_back(ui1);
	buf.push_back(ui2);
	buf.push_back(ui3);
}

std::shared_ptr<Shape> Shape::buildSphere(int v) {
	std::vector<float> posBuf;
	std::vector<float> norBuf;
	std::vector<float> texBuf;
	std::vector<unsigned int> indBuf;
	
	float r = 1;	
	for (int y=v-1; y>=0; y--) {
		for (int x=0; x<v; x++) {
			float alpha = x / (float) (v-1);
			float beta = y / (float) (v-1);
			
			float theta = M_PI * (1 - beta);
			float phi = 2*M_PI * (1 - alpha);	
			glm::vec3 p = r*glm::vec3(sin(theta)*sin(phi), cos(theta), sin(theta)*cos(phi));

			Shape::pushvec(posBuf, p.x, p.y, p.z);
			Shape::pushvec(norBuf, p.x/r, p.y/r, p.z/r);
			Shape::pushvec(texBuf, alpha * 10,	beta * 10);
			
			if (x != v-1 && y != v-1) {
				int k = x + v*y;
				Shape::pushvec(indBuf, k, k+1, k+v+1);
				Shape::pushvec(indBuf, k+v+1, k+v, k);
			}
		}
	}

	shared_ptr<Shape> sphere = make_shared<Shape>();
	sphere->loadMeshBuffers(posBuf, norBuf, texBuf, indBuf);
	sphere->init();
	return sphere;
}

float Shape::getBaseY() {
	return this->baseY;
}

void Shape::loadMeshBuffers(std::vector<float> posBuf, std::vector<float> norBuf, std::vector<float> texBuf, std::vector<unsigned int> indBuf) {
	this->procedural = true;
	this->posBuf = posBuf;
	this->norBuf = norBuf;
	this->texBuf = texBuf;
	this->indBuf = indBuf;
	
	this->baseY = FLT_MAX;
	for (size_t yi = 1; yi<posBuf.size(); yi+=3) {
		this->baseY = min(this->baseY, posBuf[yi]);
	}
}

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
	// Send the position array to the GPU
	glGenBuffers(1, &posBufID);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_STATIC_DRAW);
	
	// Send the normal array to the GPU
	if(!norBuf.empty()) {
		glGenBuffers(1, &norBufID);
		glBindBuffer(GL_ARRAY_BUFFER, norBufID);
		glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_STATIC_DRAW);
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
		glVertexAttribPointer(prog->getAttribute("aPos"), 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
	}

	int aNor = prog->getAttribute("aNor");
	if (aNor != -1 && posBufID != 0) {
		glEnableVertexAttribArray(prog->getAttribute("aNor"));
		glBindBuffer(GL_ARRAY_BUFFER, norBufID);
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
