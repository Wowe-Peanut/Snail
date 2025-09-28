#version 120

uniform mat4 P;
uniform mat4 MV;
uniform mat4 MVIT;
uniform float t;

attribute vec4 aPos; 
attribute vec3 aNor; 

varying vec3 fNor; 
varying vec3 fPos;

void main()
{
	float x = aPos.x;
	float theta = aPos.y;

	float fx = cos(x+t)+2;
	float dfdx = -sin(x+t);

	vec4 p = vec4(x, fx*cos(theta), fx*sin(theta), 1);
	vec3 dpdx = vec3(1, dfdx*cos(theta), dfdx*sin(theta));
	vec3 dpda = vec3(0, fx*-sin(theta), fx*cos(theta));
	vec3 n = -normalize(cross(dpdx, dpda));

	gl_Position = P * (MV * p);
	fNor = normalize(vec3(MVIT * vec4(n, 0.0))); 
	fPos = vec3(MV * p);
}
