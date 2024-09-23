#include "program.hpp"

#include <iostream>

#include "utility.hpp"
#include "input.hpp"



GLuint screenQuadProgram = -1;
GLuint rayTraceProgram = -1;

int renderWidth = 640;
int renderHeight = 360;



#define TexParameter \
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); \
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); \
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);   \
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST)

#define CreateTexture(name, unit, internalformat, format, type)                                           \
	GLuint name;                                                                                      \
	glGenTextures(1, &name);                                                                          \
	glActiveTexture(GL_TEXTURE ## unit);                                                              \
	glBindTexture(GL_TEXTURE_2D, name);                                                               \
	TexParameter;                                                                                     \
	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, renderWidth, renderHeight, 0, format, type, NULL); \
	glBindImageTexture(unit, name, 0, GL_FALSE, 0, GL_WRITE_ONLY, internalformat)



bool ProgramInit()
{
	GLuint vertexShader = CreateShader(GL_VERTEX_SHADER, "vert.glsl");
	GLuint fragmentShader = CreateShader(GL_FRAGMENT_SHADER, "frag.glsl");
	screenQuadProgram = CreateProgram(vertexShader, fragmentShader);
	SetUniform(screenQuadProgram, "gAlbedoSpecular", 0);
	SetUniform(screenQuadProgram, "gPosition", 1);
	SetUniform(screenQuadProgram, "gNormal", 2);
	SetUniform(screenQuadProgram, "gDepth", 3);

	GLuint computeShader = CreateShader(GL_COMPUTE_SHADER, "comp.glsl");
	rayTraceProgram = CreateProgram(computeShader);
	SetUniform(rayTraceProgram, "gAlbedoSpecular", 0);
	SetUniform(rayTraceProgram, "gPosition", 1);
	SetUniform(rayTraceProgram, "gNormal", 2);
	SetUniform(rayTraceProgram, "gDepth", 3);

	
	
	// Create the G-BUFFER oh yes

	CreateTexture(gAlbedoSpecular, 0, GL_RGBA32F, GL_RGBA, GL_FLOAT);
	CreateTexture(gPosition, 1, GL_RGBA32F, GL_RGBA, GL_FLOAT);
	CreateTexture(gNormal, 2, GL_RGBA32F, GL_RGBA, GL_FLOAT);
	CreateTexture(gDepth, 3, GL_R32F, GL_RED, GL_FLOAT);

	
	return true;
}

int frameCount = 0;
float combinedTime = 0;
void ProgramLoop()
{
	combinedTime += deltaTime;
	if ((frameCount & 0xFFF) == 0)
	{
		//std::cout << "FPS = " << 1.0f / (combinedTime / 0x1000) << "\r";
		//std::cout << "FPS = " << combinedTime << "\r";

		combinedTime = 0;
	}

	float angle = (float)frameCount / 1000.0f;
	glm::mat4 inverseProjection = glm::inverse(glm::perspective(glm::radians(110.0f), (float)renderWidth / (float)renderHeight, 0.1f, 100.0f));
	glm::vec3 cameraPosition = glm::vec3(0, 0, 0);
	//angle = 0;
	glm::vec3 cameraForward = glm::vec3(cosf(angle), 0, sinf(angle));
	glm::vec3 cameraUp = glm::vec3(0, 1, 0);
	glm::mat4 cameraToWorld = glm::lookAt(cameraPosition, cameraPosition + cameraForward, cameraUp);

	cameraToWorld = glm::transpose(cameraToWorld);
	cameraToWorld = glm::inverse(cameraToWorld);

	glUseProgram(rayTraceProgram);
	SetUniform(rayTraceProgram, "cameraToWorld", cameraToWorld);
	SetUniform(rayTraceProgram, "cameraInverseProjection", inverseProjection);

	glm::mat4 printMatrix = cameraToWorld;
	std::cout << "\n\n\nEy\n";
	std::cout << "{ " << printMatrix[0][0] << ", " << printMatrix[0][1] << ", " << printMatrix[0][2] << ", " << printMatrix[0][3] << " }" << "\n";
	std::cout << "{ " << printMatrix[1][0] << ", " << printMatrix[1][1] << ", " << printMatrix[1][2] << ", " << printMatrix[1][3] << " }" << "\n";
	std::cout << "{ " << printMatrix[2][0] << ", " << printMatrix[2][1] << ", " << printMatrix[2][2] << ", " << printMatrix[2][3] << " }" << "\n";
	std::cout << "{ " << printMatrix[3][0] << ", " << printMatrix[3][1] << ", " << printMatrix[3][2] << ", " << printMatrix[3][3] << " }" << "\n";
	glm::vec4 testo = glm::vec4(0, 0, 0, 1);
	glm::vec4 testo2 = testo * cameraToWorld;
	std::cout << "THE VECTOR IS EQUAL TO " << "{ " << testo2.x << ", " << testo2.y << ", " << testo2.z << ", " << testo2.w << " }" << "\n";

        glUseProgram(rayTraceProgram);
        glDispatchCompute((GLuint)renderWidth / 1, (GLuint)renderHeight / 1, 1);

        // make sure writing to image has finished before read
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // render image to quad
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(screenQuadProgram);
        RenderQuad();

	frameCount++;
}

void ProgramQuit()
{
}
