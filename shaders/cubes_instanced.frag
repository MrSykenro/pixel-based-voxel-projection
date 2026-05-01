#version 330 core
in float  value;
out vec4 fragment_color;

void main(){
    fragment_color = vec4(vec3(value), 1.0);
}