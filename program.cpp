#include "program.hpp"

#include <iostream>
#include <iomanip>
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>

#include "utility.hpp"
#include "input.hpp"
#include "buffer.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stbi_image.h"



GLuint screenQuadProgram = -1;
GLuint rayTraceProgram = -1;

int renderWidth = 640 / 2;
int renderHeight = 360 / 2;



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



class Camera
{
public:
	glm::vec3 m_position;
	glm::vec3 m_rotation;
	float m_fovDegrees; // Horizontal fov
	float m_aspectRatio; // Window width / Window height

	Camera()
	{
		m_position = glm::vec3(0);
		m_rotation = glm::vec3(0);
		m_fovDegrees = 90.0f;
		m_aspectRatio = 1.0f;
	}

	glm::mat4 GetViewMatrix()
	{
		glm::mat4 view = glm::identity<glm::mat4>();
		view = glm::translate(view, m_position);
		view = glm::rotate(view, m_rotation.z, glm::vec3(0, 0, 1));
		view = glm::rotate(view, m_rotation.y, glm::vec3(0, 1, 0));
		view = glm::rotate(view, m_rotation.x, glm::vec3(1, 0, 0));
		view = glm::transpose(view);
		return view;
	}
	glm::vec2 GetViewportScale()
	{
		glm::vec2 viewportScale{ 0 };
		m_aspectRatio = (float)windowWidth / (float)windowHeight;
		viewportScale.x = 1.0f;
		viewportScale.y = 1.0f / m_aspectRatio;
		viewportScale *= tanf(glm::radians(m_fovDegrees) / 2.0f);
		return viewportScale;
	}

	void UpdateMovement()
	{
		constexpr float sensitivityX = 0.005f;
		constexpr float sensitivityY = 0.004f;
		m_rotation.y += (float)mouseDeltaX * sensitivityX;
		m_rotation.x += (float)mouseDeltaY * sensitivityY;
		if (m_rotation.x < -M_PI / 2) m_rotation.x = -M_PI / 2;
		else if (m_rotation.x > M_PI / 2) m_rotation.x = M_PI / 2;
		
		glm::vec4 right   = glm::vec4(1, 0, 0, 1);
		glm::vec4 up      = glm::vec4(0, 1, 0, 1);
		glm::vec4 forward = glm::vec4(0, 0, 1, 1);
		glm::mat4 viewMatrix = GetViewMatrix();
		right   = right   * viewMatrix - glm::vec4(m_position, 0);
		up      = up      * viewMatrix - glm::vec4(m_position, 0);
		forward = forward * viewMatrix - glm::vec4(m_position, 0);

		constexpr float speed = 20.0f;
		if (inputsDown & INPUT_W)     m_position += forward * (speed * deltaTime);
		if (inputsDown & INPUT_S)     m_position -= forward * (speed * deltaTime);
		if (inputsDown & INPUT_D)     m_position += right   * (speed * deltaTime);
		if (inputsDown & INPUT_A)     m_position -= right   * (speed * deltaTime);
		if (inputsDown & INPUT_SPACE) m_position += up      * (speed * deltaTime);
		if (inputsDown & INPUT_SHIFT) m_position -= up      * (speed * deltaTime);
	}

private:


};

Camera g_camera{};



bool ProgramInit()
{
	GLuint vertexShader = CreateShader(GL_VERTEX_SHADER, "vert.glsl");
	GLuint fragmentShader = CreateShader(GL_FRAGMENT_SHADER, "frag.glsl");
	screenQuadProgram = CreateProgram(vertexShader, fragmentShader);
	SetUniform(screenQuadProgram, "gAlbedoSpecular", 0);
	SetUniform(screenQuadProgram, "gPosition", 1);
	SetUniform(screenQuadProgram, "gNormal", 2);
	SetUniform(screenQuadProgram, "gDepth", 3);
	SetUniform(screenQuadProgram, "testTexture", 4);

	GLuint computeShader = CreateShader(GL_COMPUTE_SHADER, "comp.glsl");
	rayTraceProgram = CreateProgram(computeShader);
	SetUniform(rayTraceProgram, "gAlbedoSpecular", 0);
	SetUniform(rayTraceProgram, "gPosition", 1);
	SetUniform(rayTraceProgram, "gNormal", 2);
	SetUniform(rayTraceProgram, "gDepth", 3);
	SetUniform(rayTraceProgram, "testTexture", 4);

	// Create the G-BUFFER oh yes

	renderWidth = windowWidth;
	renderHeight = windowHeight;
	renderWidth = 1920 * 1;
	renderHeight = 1080 * 1;
	CreateTexture(gAlbedoSpecular, 0, GL_RGBA32F, GL_RGBA, GL_FLOAT);
	CreateTexture(gPosition, 1, GL_RGBA32F, GL_RGBA, GL_FLOAT);
	CreateTexture(gNormal, 2, GL_RGBA32F, GL_RGBA, GL_FLOAT);
	CreateTexture(gDepth, 3, GL_R32F, GL_RED, GL_FLOAT);



	int testTextureWidth = 0;
	int testTextureHeight = 0;
	int nrChannels = 0;
	unsigned char* data = stbi_load("grass.png", &testTextureWidth, &testTextureHeight, &nrChannels, 0);
	if (!data)
	{
		std::cout << "Could not load the test texture!\n";
		return false;
	}
	GLuint testTexture;
	glGenTextures(1, &testTexture);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, testTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, testTextureWidth, testTextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(data);


	InitSphereData();
	InitModelBuffers();



	int maxWorkGroups = 0;
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxWorkGroups);
	std::cout << "glGet() = " << maxWorkGroups << "\n";

	g_camera.m_position.y = -2900;

	return true;
}





int frameCount = 0;
float combinedTime = 0;
float timeThisSecond = 0;
int framesThisSecond = 0;

std::vector<double> raytraceTimes;
std::vector<double> fragmentTimes;

void ProgramLoop()
{
	double perf_raytrace = 0.0;
	double perf_lighting = 0.0;

	g_camera.m_fovDegrees = 100.0f;
	
	g_camera.UpdateMovement();

	glUseProgram(rayTraceProgram);
	SetUniform(rayTraceProgram, "cameraToWorld", g_camera.GetViewMatrix());
	SetUniform(rayTraceProgram, "viewportScale", g_camera.GetViewportScale());

	/*glm::mat4 printMatrix = cameraToWorld;
	std::cout << "\n\n\nEy\n";
	std::cout << "{ " << printMatrix[0][0] << ", " << printMatrix[0][1] << ", " << printMatrix[0][2] << ", " << printMatrix[0][3] << " }" << "\n";
	std::cout << "{ " << printMatrix[1][0] << ", " << printMatrix[1][1] << ", " << printMatrix[1][2] << ", " << printMatrix[1][3] << " }" << "\n";
	std::cout << "{ " << printMatrix[2][0] << ", " << printMatrix[2][1] << ", " << printMatrix[2][2] << ", " << printMatrix[2][3] << " }" << "\n";
	std::cout << "{ " << printMatrix[3][0] << ", " << printMatrix[3][1] << ", " << printMatrix[3][2] << ", " << printMatrix[3][3] << " }" << "\n";
	glm::vec4 testo = glm::vec4(0, 0, 0, 1);
	glm::vec4 testo2 = testo * cameraToWorld;
	std::cout << "THE VECTOR IS EQUAL TO " << "{ " << testo2.x << ", " << testo2.y << ", " << testo2.z << ", " << testo2.w << " }" << "\n";*/

        glUseProgram(rayTraceProgram);
	perf_raytrace = glfwGetTime();
        glDispatchCompute((GLuint)renderWidth / 32, (GLuint)renderHeight / 30, 1);
	perf_raytrace = glfwGetTime() - perf_raytrace;

        // make sure writing to image has finished before read
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // render image to quad
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(screenQuadProgram);
	perf_lighting = glfwGetTime();
        RenderQuad();
	perf_lighting = glfwGetTime() - perf_lighting;

	

	combinedTime += deltaTime;
	raytraceTimes.push_back(perf_raytrace);
	fragmentTimes.push_back(perf_lighting);
	timeThisSecond += deltaTime;
	framesThisSecond++;
	if (timeThisSecond >= 1)
	{
		std::cout << std::setfill('0') << std::setw(4)
			<< std::fixed << std::setprecision(1);

		float timePerFrame = timeThisSecond / (float)framesThisSecond;
		
		double maxRaytraceTime = 0.0;
		double totRaytraceTime = 0.0;
		double avgRaytraceTime = 0.0;
		for (int i = 0; i < raytraceTimes.size(); i++)
		{
			double t = raytraceTimes[i];
			if (t > maxRaytraceTime) maxRaytraceTime = t;
			totRaytraceTime += t;
		}
		avgRaytraceTime = totRaytraceTime / raytraceTimes.size();
		double maxFragmentTime = 0.0;
		double totFragmentTime = 0.0;
		double avgFragmentTime = 0.0;
		for (int i = 0; i < fragmentTimes.size(); i++)
		{
			double t = fragmentTimes[i];
			if (t > maxFragmentTime) maxFragmentTime = t;
			totFragmentTime += t;
		}
		avgFragmentTime = totFragmentTime / fragmentTimes.size();

		std::cout << "\nPerf:\nFPS =  " << 1.0 / timePerFrame << "\n";
		std::cout << "avgRT: " << avgRaytraceTime * 1000000 << "ms\t" << "maxRT: " << maxRaytraceTime * 1000000 << "ms\n";
		std::cout << "avgFG: " << avgFragmentTime * 1000000 << "ms\t" << "maxFG: " << maxFragmentTime * 1000000 << "ms\n";
		timeThisSecond -= 1;
		framesThisSecond = 0;
		raytraceTimes.clear();
		fragmentTimes.clear();
	}



	frameCount++;
}

void ProgramQuit()
{
}
