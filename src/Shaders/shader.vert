#version 450

// ins
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;

layout(push_constant) uniform pushConstant {
	mat4 model, view, proj;
}; 

//outs
layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;

void main() {

  vec4 modelPos = model * vec4(pos, 1.0);
  gl_Position = proj * view * modelPos;
  fragPos     = vec3(modelPos);
  fragNormal  = vec3(model * vec4(normal, 0.0)); 

}