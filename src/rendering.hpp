#include <iostream>
#include <fstream>
#include <sstream>

#include <glad/glad.h>



/**
 * Compiles and returns a shader using the given files 
 */
unsigned int compileShader(const char* vertex_path, const char* fragment_path);
