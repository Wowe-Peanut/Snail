#version 120

uniform mat4 P;
uniform mat4 MV;
uniform mat4 MVIT;

attribute vec4 aPos; 
attribute vec3 aNor;

varying vec3 fPos;
varying vec3 fNor;

void main()
{
	gl_Position = P * (MV * aPos);					// Object -> Camera -> Clip
	fPos = vec3(MV * aPos);							// Object -> Camera
	fNor = normalize(vec3(MVIT * vec4(aNor, 0)));	// Object -> Camera (MVIT used to retain orthogonality)
}
