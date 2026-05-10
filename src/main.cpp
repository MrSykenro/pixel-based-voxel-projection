#include <iostream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "test.hpp"
#include "voxel_projection.hpp"
#include "rendering.hpp"

int test_cube_count = 4;
float manual_cubes[] = {
	0.0f,  0.0f, -0.5f, 0.9f,
	1.0f,  1.0f, -0.7f,  0.5f,
	-1.0f, -0.5f, -0.4f,  0.3f,
	0.0f, 0.0f, 0.0f, 1.0f
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
    CameraData camera_data[1] = {{1, 60.f, 0.f, 0.f, 0.f, 0.f, 0.f, -6.f}};
    int image_width = 5;
    int image_height = 5;
    int camera_count = 1;
    float threshold = 0.1;
    
    float difference_image[25] = {1.0f, 0.5f, 0.3f, 0.7f, 1.0f, 0.5f, 0.3f, 0.7f, 1.0f, 0.5f, 0.3f, 0.7f, 1.0f, 0.5f, 0.3f, 0.7f, 0.3f, 0.7f, 1.0f, 0.3f, 0.7f}; // Placeholder

    int pixel_count = image_height * image_width * camera_count;

	// Voxel Projection
    VoxelP::setCameraConstants(camera_data, 1);

    RayBuffer ray_buffer = VoxelP::initRayBuffer(pixel_count);

	VoxelGrid voxel_grid = VoxelP::initVoxelGrid(1, 16, -8.0f, -8.0f, 0.0f, 1, 1, 1);

    VoxelP::generateRays(difference_image, ray_buffer, threshold, image_width, image_height, camera_count);

	VoxelP::projectToGrid(ray_buffer, voxel_grid);

    VoxelP::freeRayBuffer(ray_buffer);

    // -------------------------------------------------------------------------
    // GUI
    // -------------------------------------------------------------------------
    
	size_t instance_vbo_size = sizeof(float) * 4096;

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

	unsigned int shader_program = compileShader("../shaders/cubes_instanced.vert", "../shaders/cubes_instanced.frag");

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
	glBufferData(GL_ARRAY_BUFFER, instance_vbo_size, NULL, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);

	glVertexAttribDivisor(1, 1); 
	glBindVertexArray(0); // Unbind VAO

	// HIP + OpenGL interop
	unsigned int hip_device_count = 0;
	int hip_devices; 
	HIP_CHECK(hipGLGetDevices(&hip_device_count, &hip_devices, 1, hipGLDeviceListAll));
	HIP_CHECK(hipSetDevice(hip_devices));

	hipGraphicsResource* hip_instance_vbo;
	HIP_CHECK(hipGraphicsGLRegisterBuffer(&hip_instance_vbo, instance_VBO, hipGraphicsRegisterFlagsWriteDiscard));

	HIP_CHECK(hipGraphicsMapResources(1, &hip_instance_vbo, 0));
	
	float* dptr;
	size_t size;

	HIP_CHECK(hipGraphicsResourceGetMappedPointer((void**)&dptr, &size, hip_instance_vbo));
	
	copy_to_device_array(manual_cubes, dptr, 4*test_cube_count);

	VoxelP::convertToVBO(voxel_grid, dptr);

	size_t copy_size = 1;
	float d_debug_array[1];

	HIP_CHECK(hipMemcpy(d_debug_array, voxel_grid.voxel_array, copy_size*sizeof(float), hipMemcpyDeviceToHost));
	for(int i = 0; i<copy_size; i++) std::cout << d_debug_array[i] << " ";
	std::cout << std::endl;

	HIP_CHECK(hipGraphicsUnmapResources(1, &hip_instance_vbo, 0));

	float camera_speed = 0.02f;

    // Rendering loop
    while (!glfwWindowShouldClose(window))
    {
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    	glEnable(GL_DEPTH_TEST);

		// Quit with input
		if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) break;

		// Check for input and update viewer camera settings
		if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) pitch += camera_speed;
		if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) pitch -= camera_speed;
		if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) yaw += camera_speed;
		if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) yaw -= camera_speed;
		if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) distance += camera_speed;
		if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) distance -= camera_speed;

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