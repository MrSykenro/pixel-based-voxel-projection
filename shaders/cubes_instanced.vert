#version 330 core
layout (location = 0) in vec3 object_mesh;
layout (location = 1) in vec4 instance_array;

uniform mat4 view_matrix;
uniform mat4 projection_matrix;
uniform float voxel_size;

out float value;

void main(){
    value = instance_array.w;

    vec3 world_postion = object_mesh * voxel_size + instance_array.xyz;
    gl_Position = projection_matrix * view_matrix * vec4(world_postion, 1.0);
}