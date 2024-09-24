#pragma once

#include <glad/glad.h>

extern GLuint rayTraceProgram;

bool ProgramInit();

void ProgramLoop();

void ProgramQuit();
