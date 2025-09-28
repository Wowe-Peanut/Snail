#version 120

vec2 poissonDisk[] = vec2[](
    vec2(-0.220147, 0.976896),
    vec2(-0.735514, 0.693436),
    vec2(-0.200476, 0.310353),
    vec2( 0.180822, 0.454146),
    vec2( 0.292754, 0.937414),
    vec2( 0.564255, 0.207879),
    vec2( 0.178031, 0.024583),
    vec2( 0.613912,-0.205936),
    vec2(-0.385540,-0.070092),
    vec2( 0.962838, 0.378319),
    vec2(-0.886362, 0.032122),
    vec2(-0.466531,-0.741458),
    vec2( 0.006773,-0.574796),
    vec2(-0.739828,-0.410584),
    vec2( 0.590785,-0.697557),
    vec2(-0.081436,-0.963262),
    vec2( 1.000000,-0.100160),
    vec2( 0.622430, 0.680868)
);

vec3 sampleTextureArea(sampler2D texture, vec2 tex0) {
    const int N = 18; // [1-18]
    const float blur = 0.01;
    vec3 val = vec3(0.0, 0.0, 0.0);
    for(int i = 0; i < N; i++) {
        val += texture2D(texture, tex0.xy + poissonDisk[i]*blur).rgb;
    }
    val /= N;
    return val;
}


// Lights
uniform int  lightCount;
uniform vec3 lightPositions[100];	
uniform vec3 lightColors[100];

// Attenuation
uniform float A0;
uniform float A1;
uniform float A2;

// Non-texture material uniforms (constant because we don't generate specular texture to pull from)
const vec3 ksDefault = vec3(1, 1, 1);
const float sDefault = 10;

// Input textures
uniform sampler2D posTexture;
uniform sampler2D norTexture;
uniform sampler2D keTexture;
uniform sampler2D kdTexture;
uniform vec2 windowSize;
uniform int blurOn;

void main()
{
	// Load texture values (blurring them if necessary)
	vec2 tex = vec2(gl_FragCoord.x/windowSize.x, gl_FragCoord.y/windowSize.y);
	vec3 pos = blurOn == 0 ? texture2D(posTexture, tex).xyz : sampleTextureArea(posTexture, tex);
	vec3 nor = blurOn == 0 ? texture2D(norTexture, tex).xyz : sampleTextureArea(norTexture, tex);
	vec3 ke  = blurOn == 0 ? texture2D(keTexture, tex).rgb  : sampleTextureArea(keTexture, tex);
	vec3 kd  = blurOn == 0 ? texture2D(kdTexture, tex).rgb  : sampleTextureArea(kdTexture, tex);
	
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
		fragColor += lightColors[li] * (kd*diffuse + ks*specular) / (A0 + A1*r + A2*r*r);
	}

	
	// Optionally draw individual textures as output
	//gl_FragColor = vec4(pos, 1);
	//gl_FragColor = vec4(nor, 1);
	//gl_FragColor = vec4(ke, 1);
	//gl_FragColor = vec4(kd, 1);
	gl_FragColor = vec4(fragColor, 1);	
}



