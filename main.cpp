#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "utility.hpp"
#include "input.hpp"

#include "program.hpp"



int main(int argc, char** argv)
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(640, 360, "Raytracer", NULL, NULL);
	if (!window)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
	glfwSetCursorPosCallback(window, MouseCallback);

	// glad: load all OpenGL function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "Failed to initialize GLAD" << std::endl;
		glfwTerminate();
		return -1;
	}

	if (!ProgramInit())
	{
		std::cerr << "Program failed to initialize" << std::endl;
		glfwTerminate();
		return -1;
	}

	std::cout << "Program has successfully initialized\n";



	while (!glfwWindowShouldClose(window))
	{
		ProcessInputs(window);

		// Set frame time
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		ProgramLoop();

		glfwSwapBuffers(window);
		mouseDeltaX = 0;
		mouseDeltaY = 0;
		glfwPollEvents();
	}

	ProgramQuit();

	glfwTerminate();
	return 0;
}
