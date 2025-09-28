#version 120

uniform vec3 ke;
uniform vec3 kd; 

varying vec3 fPos;
varying vec3 fNor;

void main()
{
	gl_FragData[0].xyz = fPos;
	gl_FragData[1].xyz = normalize(fNor);
	gl_FragData[2].xyz = ke;
	gl_FragData[3].xyz = kd;
}



