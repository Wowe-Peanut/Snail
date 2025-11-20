#version 120


// Lights
uniform int  lightCount;
uniform vec3 lightPositions[10];	
uniform vec3 lightColors[10];

// BPhong Qualities
uniform vec3 ka;
uniform vec3 kd; 
uniform vec3 ks;
uniform float s;

varying vec3 fPos;
varying vec3 fNor;

void main() {
	vec3 pos = fPos;
	vec3 nor = fNor;

	vec3 n = normalize(nor);
	vec3 e = normalize(-pos);  
	vec3 fragColor = ka;

	for (int li=0; li<lightCount; li++) {
		vec3 l = normalize(lightPositions[li] - pos);
		vec3 h = (e+l)/length(e+l);
		float r = distance(pos, lightPositions[li]);

		vec3 diffuse = kd*max(0, dot(l, n)); 	
		vec3 specular = ks*pow(max(0, dot(h, n)), s);

		// Ambient color just a scalar of diffuse color for now
		fragColor += lightColors[li] * (diffuse + specular);
	}

	gl_FragColor = vec4(fragColor, 1);	

	// gl_FragColor = vec4(vec3(length(pos))/5, 1);
}



