#version 330 core
layout (location = 0) in vec3 object_mesh;
layout (location = 1) in vec4 instance_array;

uniform mat4 view;
uniform mat4 projection;
uniform float voxel_size;

out float value;

void main(){
    value = instance_array.w;

    vec3 world_postion = object_mesh * voxel_size + instance_array.xyz;
    gl_Position = projection * view * vec4(world_postion, 1.0);
}