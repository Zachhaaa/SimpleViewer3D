#version 450

//ins
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;

// outs
layout(location = 0) out vec4 fragColor;


// constants
const vec4  c_cubeCol  = vec4(0.8, 0.8, 0.8, 1.0);

const float c_amb     = 0.25;
const float c_diff    = 0.6;

void main() {

	float diff = c_diff * max(dot(fragNormal, vec3(0.0, 0.0, 1.0)), 0.0);
	fragColor = (c_amb + diff) * c_cubeCol; 

}