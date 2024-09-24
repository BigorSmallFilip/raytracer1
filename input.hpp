#pragma once

#include <stdint.h>

extern int windowWidth;
extern int windowHeight;

extern float deltaTime;
extern float lastFrame;

enum
{
	INPUT_W = (1 << 0),
	INPUT_A = (1 << 1),
	INPUT_S = (1 << 2),
	INPUT_D = (1 << 3),
	INPUT_E = (1 << 4),
	INPUT_Q = (1 << 5),
	INPUT_SPACE = (1 << 6),
	INPUT_SHIFT = (1 << 7),
};

extern uint8_t inputsPressed;
extern uint8_t inputsDown;
extern uint8_t inputsReleased;

extern int mouseX;
extern int mouseY;
extern int mouseDeltaX;
extern int mouseDeltaY;



void ProcessInputs(struct GLFWwindow* window);

void MouseCallback(struct GLFWwindow* window, double posx, double posy);

void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
