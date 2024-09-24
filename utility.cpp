#include "utility.hpp"

#include <stdlib.h>



std::string ReadShaderFile(const char* filepath)
{
	// Retrieve the source code from filePath
	std::string shaderCode;
	std::ifstream shaderFile;
	// Ensure ifstream objects can throw exceptions:
	shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try
	{
		// open files
		shaderFile.open(filepath);
		std::stringstream shaderStream;
		// read file's buffer contents into streams
		shaderStream << shaderFile.rdbuf();
		// close file handlers
		shaderFile.close();
		// convert stream into string
		shaderCode = shaderStream.str();
	} catch (std::ifstream::failure& e)
	{
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
	}
	return shaderCode;
}



void CheckCompileErrors(GLuint shader, bool isProgram)
{
	GLint success;
	GLchar infoLog[1024];
	if (!isProgram)
	{
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(shader, 1024, NULL, infoLog);
			std::cout << "ERROR::SHADER_COMPILATION_ERROR\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
		}
	}
	else
	{
		glGetProgramiv(shader, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(shader, 1024, NULL, infoLog);
			std::cout << "ERROR::PROGRAM_LINKING_ERROR\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
		}
	}
}



GLuint CreateShader(GLenum type, const char* filepath)
{
	GLuint shader = glCreateShader(type);
	std::string shaderSource = ReadShaderFile(filepath);
	const char* shaderSourceStr = shaderSource.c_str();
	glShaderSource(shader, 1, &shaderSourceStr, NULL);
	glCompileShader(shader);
	CheckCompileErrors(shader, false);
	return shader;
}

GLuint CreateProgram(GLuint shader0)
{
	GLuint program = glCreateProgram();
	glAttachShader(program, shader0);
	glLinkProgram(program);
	CheckCompileErrors(program, true);
	glDeleteShader(shader0);
	return program;
}

GLuint CreateProgram(GLuint shader0, GLuint shader1)
{
	GLuint program = glCreateProgram();
	glAttachShader(program, shader0);
	glAttachShader(program, shader1);
	glLinkProgram(program);
	CheckCompileErrors(program, true);
	glDeleteShader(shader0);
	glDeleteShader(shader1);
	glUseProgram(program);
	return program;
}

void SetUniform(GLuint program, const char* const name, int value)
{
	glUniform1i(glGetUniformLocation(program, name), value);
}
void SetUniform(GLuint program, const char* const name, float value)
{
	glUniform1f(glGetUniformLocation(program, name), value);
}
void SetUniform(GLuint program, const char* const name, const glm::vec2& vec)
{
	glUniform2fv(glGetUniformLocation(program, name), 1, &vec[0]);
}
void SetUniform(GLuint program, const char* const name, const glm::mat4& mat)
{
	glUniformMatrix4fv(glGetUniformLocation(program, name), 1, GL_FALSE, &mat[0][0]);
}



unsigned int quadVAO = 0;
unsigned int quadVBO;
void RenderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}



float RandomRange(float min, float max)
{
	int r = rand() % 10000;
	float rf = (float)r / 10000.0f;
	return min + (max - min) * rf;
}
