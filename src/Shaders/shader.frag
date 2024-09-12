#version 450

//ins
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;

// outs
layout(location = 0) out vec4 fragColor;


// constants
const vec4  c_cubeCol  = vec4(0.8, 0.8, 0.8, 1.0);
const vec3  c_lightPos = vec3(4.0, -2.0, 4.0);

const float c_amb     = 0.3;
const float c_diff    = 0.7;

void main() {
	vec3  lightDir = normalize(c_lightPos - fragPos);
	float diff = c_diff * max(dot(fragNormal, lightDir), 0.0);

	fragColor = (c_amb + diff) * c_cubeCol; 

}