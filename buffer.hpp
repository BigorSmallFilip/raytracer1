#pragma once



void CreateBuffer(const char* const name, int binding, size_t datasize, void* data);



struct Sphere
{
	float position_x;
	float position_y;
	float position_z;
	float radius;
	float albedoSpecular_red;
	float albedoSpecular_green;
	float albedoSpecular_blue;
	float albedoSpecular_specular;
};

void InitSphereData();

void InitModelBuffers();
