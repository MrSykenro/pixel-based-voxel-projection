#include <iostream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "test.hpp"
#include "voxel_projection.hpp"

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

unsigned int cubeVAO, cubeVBO, instanceVBO;

std::string loadShaderSource(const char* filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
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





	float manual_cubes[] = {
    	0.0f,  0.0f, -0.5f, 100.0f, // Cube 1: Center, slightly away
    	1.0f,  1.0f, -0.7f,  50.0f, // Cube 2: Top Right, further away
   		-1.0f, -0.5f, -0.4f,  10.0f  // Cube 3: Bottom Left, very close
	};
	int test_cube_count = 3;



	// Camera Settings
	float distance = 5.0f; 
	// 45 degrees in radians for all angles
	float angle = glm::radians(45.0f); 
	
	// Calculate camera position using spherical coordinates
	// This puts the camera at a 45-degree offset on all axes
	float camX = distance * cos(angle) * sin(angle);
	float camY = distance * sin(angle);
	float camZ = distance * cos(angle) * cos(angle);
	
	glm::vec3 cameraPos   = glm::vec3(camX, camY, camZ);
	glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 up           = glm::vec3(0.0f, 1.0f, 0.0f);
	
	// Create Matrices
	glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, up);
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), 640.0f / 480.0f, 0.1f, 100.0f);


	
	// 1. Load the source code from files
	std::string vertexCode = loadShaderSource("../shaders/cubes_instanced.vert");
	std::string fragmentCode = loadShaderSource("../shaders/cubes_instanced.frag");

	const char* vShaderSource = vertexCode.c_str();
	const char* fShaderSource = fragmentCode.c_str();

	int success;
	char infoLog[512];

	// 2. Compile Vertex Shader
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vShaderSource, NULL);
	glCompileShader(vertexShader);
	
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success) {
	    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
	    std::cout << "Vertex Compilation Failed:\n" << infoLog << std::endl;
	}

	// 3. Compile Fragment Shader
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fShaderSource, NULL);
	glCompileShader(fragmentShader);
	
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success) {
	    glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
	    std::cout << "Fragment Compilation Failed:\n" << infoLog << std::endl;
	}

	// 4. Link Program
	unsigned int shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	// Check for linking errors
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
	    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
	    std::cout << "Shader Linking Failed:\n" << infoLog << std::endl;
	}

	// 5. Cleanup
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	unsigned int cubeVAO, cubeVBO, instanceVBO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);
	glGenBuffers(1, &instanceVBO);

	glBindVertexArray(cubeVAO);

	// Layout 0: Standard Cube Vertices
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Layout 1: Manual Instance Data
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	// Upload our 3 test cubes manually
	glBufferData(GL_ARRAY_BUFFER, sizeof(manual_cubes), manual_cubes, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(1, 1); 

	glBindVertexArray(0);

    // Rendering loop
    while (!glfwWindowShouldClose(window))
    {
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    	glEnable(GL_DEPTH_TEST);

    	glUseProgram(shaderProgram);

    	// Pass the Identity Matrices since we aren't using GLM
    	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		
    	// Set voxel size and max intensity
    	glUniform1f(glGetUniformLocation(shaderProgram, "voxel_size"), 0.2f);
    	glUniform1f(glGetUniformLocation(shaderProgram, "max_intensity"), 100.0f);

    	// Draw
    	glBindVertexArray(cubeVAO);
    	glDrawArraysInstanced(GL_TRIANGLES, 0, 36, test_cube_count);

    	glfwSwapBuffers(window);
    	glfwPollEvents();
    }




	
    glfwTerminate();
    return 0;
}