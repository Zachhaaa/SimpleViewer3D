#version 450

// ins
layout (location = 0) in vec3 pos;

layout(push_constant) uniform pushConstant {
	mat4 view, proj;
}; 

// moves the entire mesh forward in the z buffer so that the depth depther chooses only the triangles that show.  
#define MESH_OUTLINE_Z_OFFSET -0.0000001

void main() {

  vec4 transformed = proj * view * vec4(pos, 1.0);
  transformed.z += transformed.w * MESH_OUTLINE_Z_OFFSET; // multiply by w component to counter the perspective division
  gl_Position = transformed; 

}