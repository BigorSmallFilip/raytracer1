#include "input.hpp"

#include <GLFW/glfw3.h>



int windowWidth = 640;
int windowHeight = 360;

float deltaTime = 0;
float lastFrame = 0;

uint8_t inputsPressed = 0;
uint8_t inputsDown = 0;
uint8_t inputsDownPrev = 0;
uint8_t inputsReleased = 0;

int mouseX = 0;
int mouseY = 0;
int mouseDeltaX = 0;
int mouseDeltaY = 0;
int mouseXPrev = 0;
int mouseYPrev = 0;
bool firstMouseUpdate = true;

void ProcessInputs(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	inputsPressed = 0;
	inputsDown = 0;
	inputsReleased = 0;

	static const int glfw_keycodes[] = {
		GLFW_KEY_W, GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D,
		GLFW_KEY_E, GLFW_KEY_Q, GLFW_KEY_SPACE, GLFW_KEY_LEFT_SHIFT,
	};
	for (int i = 0; i < 8; i++)
	{
		if (glfwGetKey(window, glfw_keycodes[i]) == GLFW_PRESS)
		{
			inputsDown |= (1 << i);
			inputsPressed |= (inputsDownPrev & (1 << i)) ? 0 : (1 << i);
		}
		else
		{
			inputsReleased |= !(inputsDownPrev & (1 << i)) ? 0 : (1 << i);
		}
	}

	inputsDownPrev = inputsDown;
}

void MouseCallback(GLFWwindow* window, double posx, double posy)
{
	float posxf = (float)posx;
	float posyf = (float)posy;
	if (firstMouseUpdate)
	{
		mouseXPrev = (int)posxf;
		mouseYPrev = (int)posyf;
		firstMouseUpdate = false;
	}

	mouseDeltaX = (int)posxf - mouseXPrev;
	mouseDeltaY = (int)posyf - mouseYPrev;
	mouseXPrev = (int)posxf;
	mouseYPrev = (int)posyf;
	mouseX = (int)posxf;
	mouseY = (int)posyf;
}

void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
	windowWidth = width;
	windowHeight = height;
}
