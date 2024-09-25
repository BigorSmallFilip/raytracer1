#include "buffer.hpp"

#include <glad/glad.h>

#include "program.hpp"
#include "input.hpp"
#include "utility.hpp"
#include "bvh.hpp"



void CreateBuffer(const char* const name, int binding, size_t datasize, void* data)
{
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, datasize, data, GL_STATIC_DRAW);

	int block_index = glGetProgramResourceIndex(rayTraceProgram, GL_SHADER_STORAGE_BLOCK, name);
	glShaderStorageBlockBinding(rayTraceProgram, block_index, binding);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, buffer);
}

void CreateBufferAndCount(const char* const name, int binding, size_t datasize, void* data)
{
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, datasize + 16, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 4, &datasize);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 16, datasize, data);

	int block_index = glGetProgramResourceIndex(rayTraceProgram, GL_SHADER_STORAGE_BLOCK, name);
	glShaderStorageBlockBinding(rayTraceProgram, block_index, binding);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, buffer);
}



void InitSphereData()
{
	struct
	{
		int sphereCount = 20;
		int PADDING0;
		int PADDING1;
		int PADDING2;
		Sphere spheres[20] = { 0 };
	} ssbo_bufferdata;

	for (int i = 0; i < 20; i++)
	{
		ssbo_bufferdata.spheres[i] = {
			RandomRange(-10, 10),
			RandomRange(-10, 10),
			RandomRange(-10, 10),
			RandomRange(0.5f, 2.0f),
			RandomRange(0, 1),
			RandomRange(0, 1),
			RandomRange(0, 1),
			1
		};
	}

	CreateBuffer("sphere_buffer", 5, sizeof(ssbo_bufferdata), &ssbo_bufferdata);
}



void InitModelBuffers()
{
	BuildAndDoEverythingElseWithBVH();

	return;
}
