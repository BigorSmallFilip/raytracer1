#pragma once

#include <vector>

struct float3
{
	float x, y, z;
};

struct Triangle
{
	float3 vertA, vertB, vertC; float padding0;
	float3 normA, normB, normC; float padding1;
};

struct Model
{
	std::vector<Triangle> triangles;
	float3 albedo;
	float specular;
};

Model LoadModel(const char* const filepath);

bool BuildAndDoEverythingElseWithBVH();
