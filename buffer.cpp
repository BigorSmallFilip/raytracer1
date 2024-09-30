#include "buffer.hpp"

#include <glad/glad.h>

#include "program.hpp"
#include "input.hpp"
#include "utility.hpp"
#include "bvh.hpp"



void CreateBuffer(const char* const name, int binding, int datasize, void* data)
{
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, datasize, data, GL_STATIC_DRAW);

	int block_index = glGetProgramResourceIndex(rayTraceProgram, GL_SHADER_STORAGE_BLOCK, name);
	glShaderStorageBlockBinding(rayTraceProgram, block_index, binding);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, buffer);
}

void CreateBufferAndCount(const char* const name, int binding, int numelements, int datasize, void* data)
{
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, datasize + 16, NULL, GL_STATIC_DRAW);

	void* ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
	// now copy data into memory
	memcpy(ptr, &numelements, sizeof(int));
	memcpy((void*)((size_t)ptr + 16), data, datasize);
	// make sure to tell OpenGL we're done with the pointer
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	int block_index = glGetProgramResourceIndex(rayTraceProgram, GL_SHADER_STORAGE_BLOCK, name);
	glShaderStorageBlockBinding(rayTraceProgram, block_index, binding);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, buffer);

}



void InitSphereData()
{
	Sphere spheres[100] = { 0 };

	for (int i = 0; i < 100; i++)
	{
		spheres[i] = {
			RandomRange(-40, 40),
			RandomRange(-40, 40),
			RandomRange(-40, 40),
			RandomRange(0.5f, 2.0f),
			RandomRange(0, 1),
			RandomRange(0, 1),
			RandomRange(0, 1),
			1
		};
	}

	CreateBufferAndCount("sphere_buffer", 4, 0, sizeof(spheres), &spheres);
}



void InitModelBuffers()
{
	BuildAndDoEverythingElseWithBVH();

	return;
}
