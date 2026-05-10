#include "rendering.hpp"

// Fetch contents of the specified shader source file
std::string loadShaderSource(const char* filePath){
	std::ifstream file(filePath);
	if(!file.is_open()){
		std::cerr << "Failed to open shader file: " << filePath << std::endl;
		return "";
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

unsigned int compileShader(const char* vertex_path, const char* fragment_path){
	// Load shader code
	std::string vertex_code = loadShaderSource(vertex_path);
	std::string fragment_code = loadShaderSource(fragment_path);
	const char* v_shader_source = vertex_code.c_str();
	const char* f_shader_source = fragment_code.c_str();

	int success;
	char info_log[512];

	// Compile vertex shader
	unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &v_shader_source, NULL);
	glCompileShader(vertex_shader);

	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	if(!success){
		glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
		std::cout << "Vertex compilation failed:\n" << info_log << std::endl;
	}

	// Compile fragment shader
	unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &f_shader_source, NULL);
	glCompileShader(fragment_shader);

	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	if(!success){
		glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
		std::cout << "Fragment compilation failed:\n" << info_log << std::endl;
	}

	// Link the program
	unsigned int shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);

	glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
	if(!success){
		glGetProgramInfoLog(shader_program, 512, NULL, info_log);
		std::cout << "Shader linking failed:\n" << info_log << std::endl;
	}

	// Cleanup
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

    return shader_program;
}