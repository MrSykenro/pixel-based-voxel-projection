#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "test.hpp"
#include "voxel_projection.hpp"

int test_cube_count = 3;
float manual_cubes[] = {
	0.0f,  0.0f, -0.5f, 1.0f,
	1.0f,  1.0f, -0.7f,  0.5f,
	-1.0f, -0.5f, -0.4f,  0.3f
};

float cube_vertices[] = {
    // Back face
    -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
    // Front face
    -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
    // Left face
    -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
    // Right face
     0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
    // Bottom face
    -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
    // Top face
    -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f,
};

// Initialize viewer camera settings
float distance = 5.0f; 
float pitch = 0.0f;
float yaw = 0.0f;

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

int main() {
    // -------------------------------------------------------------------------
    // For Testing - Unrelated
    // -------------------------------------------------------------------------

    int N = 100;
    std::vector<float> a(N, 10.0f), b(N, 2.0f), c(N, 0.0f);

    // Call the wrapper
    run_vector_add(a, b, c, N);

    std::cout << "Result of first element: " << c[0] << std::endl;

    // -------------------------------------------------------------------------
    // Pixel-Based-Voxel-Projection
    // -------------------------------------------------------------------------

    // Fake Data
    // Needs a way to take frames
    CameraData camera_data[1] = {{1, 60.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f}};
    int image_width = 1920;
    int image_height = 1080;
    int camera_count = 1;
    float threshold = 0.1;
    
    float* difference_image = nullptr; // Placeholder

    int pixel_count = image_height * image_width * camera_count;

    VoxelP::setCameraConstants(camera_data, 1);

    RayBuffer ray_buffer = VoxelP::initRayBuffer(pixel_count);

    VoxelP::generateRays(difference_image, ray_buffer, threshold, image_width, image_height, camera_count);

    VoxelP::freeRayBuffer(ray_buffer);

    // -------------------------------------------------------------------------
    // GUI
    // -------------------------------------------------------------------------
    
    // Initialize GLFW
    if (!glfwInit()) return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create window
    GLFWwindow* window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

	// Load GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
    }

	// Load shader code
	std::string vertex_code = loadShaderSource("../shaders/cubes_instanced.vert");
	std::string fragment_code = loadShaderSource("../shaders/cubes_instanced.frag");
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

	// Create VAO and VBO's
	unsigned int voxels_VAO, object_VBO, instance_VBO;
	glGenVertexArrays(1, &voxels_VAO);
	glGenBuffers(1, &object_VBO);
	glGenBuffers(1, &instance_VBO);

	glBindVertexArray(voxels_VAO); // Bind VAO

	// Load object data. Layout 0
	glBindBuffer(GL_ARRAY_BUFFER, object_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Load instance data. Layout 1
	glBindBuffer(GL_ARRAY_BUFFER, instance_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(manual_cubes), manual_cubes, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);

	glVertexAttribDivisor(1, 1); 
	glBindVertexArray(0); // Unbind VAO

	float camera_speed = 0.02f;

    // Rendering loop
    while (!glfwWindowShouldClose(window))
    {
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    	glEnable(GL_DEPTH_TEST);
		
		// Check for input and update viewer camera settings
		if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) distance += camera_speed;
		if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) distance -= camera_speed;
		if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) yaw += camera_speed;
		if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) yaw -= camera_speed;
		if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) pitch += camera_speed;
		if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) pitch -= camera_speed;

		// Clamp pitch value
		if(pitch > 1.5f) pitch = 1.5f;
		if(pitch < -1.5f) pitch = -1.5f;

		// Calculate camera position
		float camera_x = distance * cos(pitch) * sin(yaw);
		float camera_y = distance * sin(pitch);
		float camera_z = distance * cos(pitch) * cos(yaw);

		// Calculate matrices
		glm::vec3 camera_position = glm::vec3(camera_x, camera_y, camera_z);
		glm::vec3 camera_target = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 up_vector = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::mat4 view_matrix = glm::lookAt(camera_position, camera_target, up_vector);
		glm::mat4 projection_matrix = glm::perspective(glm::radians(60.0f), 640.0f/480.0f, 0.1f, 100.0f);

		// Call the shader with the given settings
		glUseProgram(shader_program);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "view_matrix"), 1, GL_FALSE, glm::value_ptr(view_matrix));
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection_matrix"), 1, GL_FALSE, glm::value_ptr(projection_matrix));
		glUniform1f(glGetUniformLocation(shader_program, "voxel_size"), 0.2f);

		glBindVertexArray(voxels_VAO); // Bind VAO
		glDrawArraysInstanced(GL_TRIANGLES, 0, 36, test_cube_count);

    	glfwSwapBuffers(window);
    	glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}