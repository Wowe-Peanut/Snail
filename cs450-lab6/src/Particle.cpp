#include "Particle.h"

#include <iostream>

#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Texture.h"

using namespace std;
using namespace Eigen;

vector<float> Particle::posBuf;
vector<float> Particle::colBuf;
vector<float> Particle::alpBuf;
vector<float> Particle::scaBuf;
GLuint Particle::posBufID;
GLuint Particle::colBufID;
GLuint Particle::alpBufID;
GLuint Particle::scaBufID;

// Before this constructor is called, posBuf must be a valid vector<float>.
// I.e., Particle::init(n) must be called first.
Particle::Particle(int index) :
	color(&colBuf[3*index]),
	scale(scaBuf[index]),
	x(&posBuf[3*index]),
	alpha(alpBuf[index])
{
	// Random fixed properties
	color << randFloat(0.5f, 1.0f), randFloat(0.5f, 1.0f), randFloat(0.5f, 1.0f);
	scale = randFloat(0.1f, 0.2f);
	
	// Send color data to GPU
	glBindBuffer(GL_ARRAY_BUFFER, colBufID);
	glBufferSubData(GL_ARRAY_BUFFER, 3*index*sizeof(float), 3*sizeof(float), color.data());
	
	// Send scale data to GPU
	glBindBuffer(GL_ARRAY_BUFFER, scaBufID);
	glBufferSubData(GL_ARRAY_BUFFER, index*sizeof(float), sizeof(float), &scale);
}

Particle::~Particle()
{
}

void Particle::rebirth(float t, const bool *keyToggles)
{
	m = 1.0f;
	alpha = 1.0f;
	
	//
	// <IMPLEMENT ME>
	// Replace these initial conditions
	//
	
	if(keyToggles[(unsigned)'g']) {
		// Gravity towards origin
		d = 0; // randFloat(0.1, 0.2f);

		float theta = randFloat(0, 2.0f*M_PI);
		float r = randFloat(0.5f, 1.5f);

		x << r*cos(theta), 0, r*sin(theta);
		v << x[2], 0, -x[0];
		lifespan = 50.0f;

	} else {
		// Gravity downwards
		d = randFloat(0.1, 0.2f);
		x << randFloat(-0.1f, 0.1f), randFloat(-0.1f, 0.1f), randFloat(-0.1f, 0.1f);
		v << randFloat(-1.0f, 1.0f), randFloat(5.0f, 6.0f), randFloat(-1.0f, 1.0f);
		lifespan = randFloat(1.5f, 2.5f);

	}

	
	//
	// </IMPLEMENT ME>
	//

	tEnd = t + lifespan;
}

void Particle::step(float t, float h, const Vector3f &g, const bool *keyToggles)
{
	if(t > tEnd) {
		rebirth(t, keyToggles);
	}
	// Update alpha based on current time
	alpha = (tEnd-t)/lifespan;
	
	// Central gravity constants
	float C = 5;
	float eps = 0.2f;

	Eigen::Vector3f netForce(0, 0, 0);
	
	// Damping force
	netForce += -d * v; // Some small force opposite velocity is applied that is proportional to v
	
	if(keyToggles[(unsigned)'g']) {
		// Gravity towards origin

		netForce += -x * C * m / pow((x.squaredNorm() + eps*eps), 1.5f);

	} else {
		// Gravity downwards
		netForce += g * m;
	}
	
	//
	// </IMPLEMENT ME>
	//
	
	// Update position
	v = v + h*netForce/m; // f=ma, so a=f/m and d times delta t (h here) is our new veclocity
	x += h*v; 	// Symplectic Euler uses the new velocity to update the old position rather than the old velocity
	
	// Apply floor collision
	float floorHeight = -1;

	if(keyToggles[(unsigned)'f']) {
		if (x[1] < floorHeight) {

			// Cap y at floor and then rebout with some energy loss
			x[1] = floorHeight;
			if (v[1] < 0.0f) {
				v[1] = -v[1];
			}
		}
	}
}

float Particle::randFloat(float l, float h)
{
	float r = rand() / (float)RAND_MAX;
	return (1.0f - r) * l + r * h;
}

void Particle::init(int n)
{
	posBuf.resize(3*n);
	colBuf.resize(3*n);
	alpBuf.resize(n);
	scaBuf.resize(n);
	
	for(int i = 0; i < n; ++i) {
		posBuf[3*i+0] = 0.0f;
		posBuf[3*i+1] = 0.0f;
		posBuf[3*i+2] = 0.0f;
		colBuf[3*i+0] = 1.0f;
		colBuf[3*i+1] = 1.0f;
		colBuf[3*i+2] = 1.0f;
		alpBuf[i] = 1.0f;
		scaBuf[i] = 1.0f;
	}

	// Generate buffer IDs
	GLuint bufs[4];
	glGenBuffers(4, bufs);
	posBufID = bufs[0];
	colBufID = bufs[1];
	alpBufID = bufs[2];
	scaBufID = bufs[3];
	
	// Send color buffer to GPU
	glBindBuffer(GL_ARRAY_BUFFER, colBufID);
	glBufferData(GL_ARRAY_BUFFER, colBuf.size()*sizeof(float), &colBuf[0], GL_STATIC_DRAW);
	
	// Send scale buffer to GPU
	glBindBuffer(GL_ARRAY_BUFFER, scaBufID);
	glBufferData(GL_ARRAY_BUFFER, scaBuf.size()*sizeof(float), &scaBuf[0], GL_STATIC_DRAW);
	
	assert(glGetError() == GL_NO_ERROR);
}

void Particle::draw(const vector< shared_ptr<Particle> > &particles,
					shared_ptr<Program> prog)
{
	// Enable, bind, and send position array
	glEnableVertexAttribArray(prog->getAttribute("aPos"));
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_DYNAMIC_DRAW);
	glVertexAttribPointer(prog->getAttribute("aPos"), 3, GL_FLOAT, GL_FALSE, 0, 0);
	
	// Enable, bind, and send alpha array
	glEnableVertexAttribArray(prog->getAttribute("aAlp"));
	glBindBuffer(GL_ARRAY_BUFFER, alpBufID);
	glBufferData(GL_ARRAY_BUFFER, alpBuf.size()*sizeof(float), &alpBuf[0], GL_DYNAMIC_DRAW);
	glVertexAttribPointer(prog->getAttribute("aAlp"), 1, GL_FLOAT, GL_FALSE, 0, 0);
	
	// Enable and bind color array
	glEnableVertexAttribArray(prog->getAttribute("aCol"));
	glBindBuffer(GL_ARRAY_BUFFER, colBufID);
	glVertexAttribPointer(prog->getAttribute("aCol"), 3, GL_FLOAT, GL_FALSE, 0, 0);
	
	// Enable and bind scale array
	glEnableVertexAttribArray(prog->getAttribute("aSca"));
	glBindBuffer(GL_ARRAY_BUFFER, scaBufID);
	glVertexAttribPointer(prog->getAttribute("aSca"), 1, GL_FLOAT, GL_FALSE, 0, 0);
	
	// Draw
	glDrawArrays(GL_POINTS, 0, 3*particles.size());
	
	// Disable and unbind
	glDisableVertexAttribArray(prog->getAttribute("aSca"));
	glDisableVertexAttribArray(prog->getAttribute("aCol"));
	glDisableVertexAttribArray(prog->getAttribute("aAlp"));
	glDisableVertexAttribArray(prog->getAttribute("aPos"));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
