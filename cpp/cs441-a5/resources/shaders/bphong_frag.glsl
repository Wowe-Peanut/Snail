#version 120


// Lights
uniform int  lightCount;
uniform vec3 lightPositions[100];	
uniform vec3 lightColors[100];

// Non-texture material uniforms (constant because we don't generate specular texture to pull from)
const vec3 ksDefault = vec3(1, 1, 1);
const float sDefault = 10;

uniform vec3 ke;
uniform vec3 kd; 

varying vec3 fPos;
varying vec3 fNor;

void main()
{
	vec3 pos = fPos;
	vec3 nor = fNor;
	
	// zero ke means its a light object and should not have zero specular
	vec3 ks = ke != vec3(0) ? vec3(0) : ksDefault;
	float s = ke != vec3(0) ? 0 : sDefault;

	vec3 n = normalize(nor);
	vec3 e = normalize(-pos);  
	vec3 fragColor = ke;

	for (int li=0; li<lightCount; li++) {
		vec3 l = normalize(lightPositions[li] - pos);
		vec3 h = (e+l)/length(e+l);
		float r = distance(pos, lightPositions[li]);

		vec3 diffuse = kd*max(0, dot(l, n)); 	
		vec3 specular = ks*pow(max(0, dot(h, n)), s);
		fragColor += kd/10 + lightColors[li] * (kd*diffuse + ks*specular);
	}

	gl_FragColor = vec4(fragColor, 1);	
}



