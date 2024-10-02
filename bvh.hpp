#pragma once

#include <vector>
#include <glm/glm.hpp>

struct Triangle
{
	glm::vec4 vertA, vertB, vertC;
	glm::vec4 normA, normB, normC;
};
struct TinyTriangle
{
	unsigned int Avx_Avy;
	unsigned int Bvx_Bvy;
	unsigned int Cvx_Cvy;
	unsigned int Avz_Bvz;
	unsigned int Cvz_Anx_Any;
	unsigned int Anz_Bnx_Bny_Bnz;
	unsigned int Cnx_Cny_Cnz;
	unsigned int _padding;
};

struct Model
{
	std::vector<Triangle> triangles;
	glm::vec3 albedo;
	float specular;
};

struct BoundingBox
{
	glm::vec3 min;
	glm::vec3 max;
	glm::vec3 Center() const;
	glm::vec3 Size() const;
	bool hasPoint;

	void GrowToInclude(glm::vec3 min, glm::vec3 max);
};

// Bottom-level acceleration structure
// Stores the triangle and bvh for a single model
struct BLAS
{
	struct Node
	{
		glm::vec3 boundsMin; float _padding0;
		glm::vec3 boundsMax; float _padding1;
		// Index of first child (if triangle count is negative) otherwise index of first triangle
		int startIndex;
		int triangleCount;
		int _padding2;
		int _padding3;

		Node(BoundingBox bounds);
		Node(BoundingBox bounds, int _startIndex, int _triangleCount);
		glm::vec3 CalculateBoundsSize();
		glm::vec3 CalculateBoundsCentre();
	};
	struct TinyNode
	{
		unsigned int mix_max;
		unsigned int miy_may;
		unsigned int miz_maz;
		unsigned int startIndex24_triangleCount8;
	};
	struct BVHTriangle
	{
		glm::vec3 min;
		glm::vec3 max;
		glm::vec3 center;
		int index;
		BVHTriangle(glm::vec3 _min, glm::vec3 _max, glm::vec3 _center, int _index) :
			min{ _min },
			max{ _max },
			center{ _center },
			index{ _index }
		{}
	};
	struct NodeList
	{
		std::vector<Node> nodes;
		int currentIndex;

		int Add(Node node);
	};
	
	BoundingBox m_bounds;
	std::vector<BVHTriangle> m_bvhtriangles;
	std::vector<Triangle> m_orderedTriangles;
	NodeList m_nodes;
	int m_maxNodeDepth;

	BLAS(const Model& model, int maxNodeDepth);

private:
	void Split(
		int parentIndex,
		const std::vector<Triangle>& triangles,
		int triGlobalStart,
		int triNum,
		int depth = 0);

	void ChooseSplit(
		int* out_axis,
		float* out_pos,
		float* out_cost,
		const Node& node,
		int start,
		int count);

	float EvaluateSplit(int splitAxis, float splitPos, int start, int count);

	float NodeCost(glm::vec3 size, int numTriangles);

};



struct RayTraceModel
{
	int nodeOffset;
	int triOffset;
	int _padding0;
	int _padding1;
	glm::mat4 worldToLocalMatrix;
	glm::mat4 localToWorldMatrix;
	glm::vec4 albedoSpecular;
	int flags;
	int _padding2;
	int _padding3;
	int _padding4;

	RayTraceModel(
		const BLAS& model_blas,
		glm::vec3 albedo = glm::vec3(1.0f, 0.0f, 0.5f),
		float specular = 0.5f,
		int flags = 0,
		glm::vec3 position = glm::vec3(0.0f),
		glm::vec3 rotation = glm::vec3(0.0f),
		glm::vec3 scale = glm::vec3(0.0f));
};



Model LoadModel(const char* const filepath);

bool BuildAndDoEverythingElseWithBVH();
