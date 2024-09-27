#version 450

// ins
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;

layout(push_constant) uniform pushConstant {
	mat4 view, proj;
}; 

//outs
layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;

void main() {

  gl_Position = proj * view * vec4(pos, 1.0);

  fragPos     = pos;
  fragNormal  = mat3(view) * normal; 

}