#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>



std::string ReadShaderFile(const char* filepath);
void CheckCompileErrors(GLuint shader, bool isProgram);
GLuint CreateShader(GLenum type, const char* filepath);
GLuint CreateProgram(GLuint shader0);
GLuint CreateProgram(GLuint shader0, GLuint shader1);

void SetUniform(GLuint program, const char* const name, int value);
void SetUniform(GLuint program, const char* const name, float value);
void SetUniform(GLuint program, const char* const name, const glm::vec2& vec);
void SetUniform(GLuint program, const char* const name, const glm::mat4& mat);

void RenderQuad();

float Lerp(float a, float b, float t);
float RandomRange(float min, float max);
