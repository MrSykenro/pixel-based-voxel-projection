#include <iostream>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "test.hpp"
#include "voxel_projection.hpp"


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
    

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    GLFWwindow* window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
    }


    const char* vertexShaderSrc =
		"#version 330 core\n"
		"layout (location = 0) in vec3 aPos;\n"
		"void main() {\n"
		"    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0f);\n" 
		"}\0";

	const char* fragmentShaderSrc =
		"#version 330 core\n"
		"out vec4 fragColor;\n"
		"void main() {\n"
		"    fragColor = vec4(0.847f, 0.247f, 0.192f, 1.0f);\n"
		"}\0";

	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSrc, 0);
	glCompileShader(vertexShader);
	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, 0, infoLog);
		std::cout << "Failed to compile the vertex shader! ERR: " << infoLog << std::endl;
	}

	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSrc, 0);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, 0, infoLog);
		std::cout << "Failed to compile the fragment shader! ERR: " << infoLog << std::endl;
	}

	unsigned int shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, 0, infoLog);
		std::cout << "Failed to link the shader program! ERR: " << infoLog << std::endl;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	float vertices[] = {
		-1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
	};

	unsigned int VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);


    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

		glClearColor(0.914f, 0.722f, 0.141f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(shaderProgram);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}