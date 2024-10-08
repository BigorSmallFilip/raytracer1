#include "bvh.hpp"

#include <glm/glm.hpp>

#include <iostream>
#include <string>
#include <sstream>
#include <strstream>

#include "program.hpp"
#include "utility.hpp"
#include "buffer.hpp"



glm::vec3 BoundingBox::Center() const
{
	return (min + max) / 2.0f;
}

glm::vec3 BoundingBox::Size() const
{
	return max - min;
}

void BoundingBox::GrowToInclude(glm::vec3 min, glm::vec3 max)
{
	if (hasPoint)
	{
		this->min.x = min.x < this->min.x ? min.x : this->min.x;
		this->min.y = min.y < this->min.y ? min.y : this->min.y;
		this->min.z = min.z < this->min.z ? min.z : this->min.z;
		this->max.x = max.x > this->max.x ? max.x : this->max.x;
		this->max.y = max.y > this->max.y ? max.y : this->max.y;
		this->max.z = max.z > this->max.z ? max.z : this->max.z;
	}
	else
	{
		hasPoint = true;
		this->min = min;
		this->max = max;
	}
}



BLAS::Node::Node(BoundingBox bounds)
{
	boundsMin = bounds.min;
	boundsMax = bounds.max;
	startIndex = 0;
	triangleCount = 0;
}

BLAS::Node::Node(BoundingBox bounds, int _startIndex, int _triangleCount)
{
	boundsMin = bounds.min;
	boundsMax = bounds.max;
	startIndex = _startIndex;
	triangleCount = _triangleCount;
}

glm::vec3 BLAS::Node::CalculateBoundsSize()
{
	return boundsMax - boundsMin;
}

glm::vec3 BLAS::Node::CalculateBoundsCentre()
{
	return (boundsMin + boundsMax) / 2.0f;
}

int BLAS::NodeList::Add(Node node)
{
	int nodeIndex = currentIndex;
	nodes.push_back(node);
	currentIndex++;
	return nodeIndex;
}




BLAS::BLAS(const Model& model, int maxNodeDepth)
{
	std::cout << "Creating BLAS\n";

	m_maxNodeDepth = maxNodeDepth;
	m_bounds = {};
	m_nodes = {};
	m_bvhtriangles.reserve(model.triangles.size());
	for (int i = 0; i < model.triangles.size(); i++)
	{
		const Triangle& tri = model.triangles[i];
		glm::vec3 boundsMin = glm::min(glm::min(tri.vertA, tri.vertB), tri.vertC);
		glm::vec3 boundsMax = glm::max(glm::max(tri.vertA, tri.vertB), tri.vertC);
		glm::vec3 center = (tri.vertA + tri.vertB + tri.vertC) / 3.0f;
		m_bvhtriangles.push_back(BVHTriangle(boundsMin, boundsMax, center, i));
		m_bounds.GrowToInclude(boundsMin, boundsMax);
	}

	m_nodes.Add(Node(m_bounds));
	Split(0, model.triangles, 0, m_bvhtriangles.size());

	for (int i = 0; i < m_bvhtriangles.size(); i++)
	{
		BVHTriangle bvhtri = m_bvhtriangles[i];
		Triangle tri = model.triangles[bvhtri.index];
		m_orderedTriangles.push_back(tri);
	}



	int startIndexMax = 0;
	int triangleCountMax = 0;
	for (int i = 0; i < m_nodes.nodes.size(); i++)
	{
		BLAS::Node node = m_nodes.nodes[i];
		if (node.startIndex > startIndexMax) startIndexMax = node.startIndex;
		if (node.triangleCount > triangleCountMax) triangleCountMax = node.triangleCount;
	}
	std::cout << "startIndexMax = " << startIndexMax << "\n";
	std::cout << "triangleCountMax = " << triangleCountMax << "\n";

	if (startIndexMax > (1 << 24) || triangleCountMax > (1 << 8))
	{
		std::cerr << "CRITICAL ERROR TOO BIG MODEL!!!\n";
		exit(-1);
	}

	std::cout << "BLAS Done!\n";
}

void BLAS::Split(
	int parentIndex,
	const std::vector<Triangle>& triangles,
	int triGlobalStart,
	int triNum,
	int depth)
{
	Node parent = m_nodes.nodes[parentIndex];
	glm::vec3 size = parent.CalculateBoundsSize();
	float parentCost = NodeCost(size, triNum);

	int splitAxis = 0;
	float splitPos = 0;
	float cost = 0;
	ChooseSplit(&splitAxis, &splitPos, &cost, parent, triGlobalStart, triNum);

	if (cost < parentCost && depth < m_maxNodeDepth)
	{
		BoundingBox boundsLeft{};
		BoundingBox boundsRight{};
		int numOnLeft = 0;

		for (int i = triGlobalStart; i < triGlobalStart + triNum; i++)
		{
			BVHTriangle tri = m_bvhtriangles[i];
			if (tri.center[splitAxis] < splitPos)
			{
				boundsLeft.GrowToInclude(tri.min, tri.max);

				BVHTriangle swap = m_bvhtriangles[triGlobalStart + numOnLeft];
				m_bvhtriangles[triGlobalStart + numOnLeft] = tri;
				m_bvhtriangles[i] = swap;
				numOnLeft++;
			}
			else
			{
				boundsRight.GrowToInclude(tri.min, tri.max);
			}
		}

		int numOnRight = triNum - numOnLeft;
		int triStartLeft = triGlobalStart + 0;
		int triStartRight = triGlobalStart + numOnLeft;

		// Split parent into two children
		int childIndexLeft = m_nodes.Add(Node(boundsLeft, triStartLeft, 0));
		int childIndexRight = m_nodes.Add(Node(boundsRight, triStartRight, 0));

		// Update parent
		parent.startIndex = childIndexLeft;
		m_nodes.nodes[parentIndex] = parent;
		//stats.RecordNode(depth, false);

		// Recursively split children
		Split(childIndexLeft, triangles, triGlobalStart, numOnLeft, depth + 1);
		Split(childIndexRight, triangles, triGlobalStart + numOnLeft, numOnRight, depth + 1);
	}
	else
	{
		// Parent is actually leaf, assign all triangles to it
		parent.startIndex = triGlobalStart;
		parent.triangleCount = triNum;
		m_nodes.nodes[parentIndex] = parent;
		//stats.RecordNode(depth, true, triNum);
	}

}

void BLAS::ChooseSplit(
	int* out_axis,
	float* out_pos,
	float* out_cost,
	const Node& node,
	int start,
	int count)
{
	if (count <= 1)
	{
		*out_axis = 0;
		*out_pos = 0;
		*out_cost = INFINITY;
		return;
	}

	float bestSplitPos = 0;
	int bestSplitAxis = 0;
	const int numSplitTests = 5;

	float bestCost = INFINITY;

	// Estimate best split pos
	for (int axis = 0; axis < 3; axis++)
	{
		for (int i = 0; i < numSplitTests; i++)
		{
			float splitT = (i + 1) / (numSplitTests + 1.0f);
			float splitPos = Lerp(node.boundsMin[axis], node.boundsMax[axis], splitT);
			float cost = EvaluateSplit(axis, splitPos, start, count);
			if (cost < bestCost)
			{
				bestCost = cost;
				bestSplitPos = splitPos;
				bestSplitAxis = axis;
			}
		}
	}

	*out_axis = bestSplitAxis;
	*out_pos = bestSplitPos;
	*out_cost = bestCost;
}

float BLAS::EvaluateSplit(int splitAxis, float splitPos, int start, int count)
{
	BoundingBox boundsLeft{};
	BoundingBox boundsRight{};
	int numOnLeft = 0;
	int numOnRight = 0;

	for (int i = start; i < start + count; i++)
	{
		BVHTriangle tri = m_bvhtriangles[i];
		if (tri.center[splitAxis] < splitPos)
		{
			boundsLeft.GrowToInclude(tri.min, tri.max);
			numOnLeft++;
		}
		else
		{
			boundsRight.GrowToInclude(tri.min, tri.max);
			numOnRight++;
		}
	}

	float costA = NodeCost(boundsLeft.Size(), numOnLeft);
	float costB = NodeCost(boundsRight.Size(), numOnRight);
	return costA + costB;
}

float BLAS::NodeCost(glm::vec3 size, int numTriangles)
{
	float halfArea = size.x * size.y + size.x * size.z + size.y * size.z;
	return halfArea * numTriangles;
}




RayTraceModel::RayTraceModel(
	const BLAS& model_blas,
	glm::vec3 albedo,
	float specular,
	int flags,
	glm::vec3 position,
	glm::vec3 rotation,
	glm::vec3 scale)
{
	nodeOffset = 0;
	triOffset = 0;
	albedoSpecular = glm::vec4(albedo, specular);
	this->flags = flags;
	glm::mat4 w = glm::mat4(1.0f);
	w = glm::scale(w, 1.0f / scale);
	w = glm::rotate(w, rotation.z, glm::vec3(0, 0, 1));
	w = glm::rotate(w, rotation.y, glm::vec3(0, 1, 0));
	w = glm::rotate(w, rotation.x, glm::vec3(1, 0, 0));
	w = glm::translate(w, -position);
	w = glm::transpose(w);
	worldToLocalMatrix = w;
	localToWorldMatrix = glm::inverse(w);
}






Model LoadModel(const char* const filepath)
{
	std::ifstream f(filepath);
	if (!f.is_open())
		return Model();

	Model model{};

	// Local cache of verts
	std::vector<glm::vec4> verts;
	std::vector<glm::vec4> normals;

	while (!f.eof())
	{
		char line[128];
		f.getline(line, 128);

		std::strstream s;
		s << line;

		char junk;

		if (line[0] == 'v' && line[1] == ' ')
		{
			glm::vec4 v;
			s >> junk >> v.x >> v.y >> v.z;
			v.x = -v.x;
			verts.push_back(v);
		}
		if (line[0] == 'v' && line[1] == 'n')
		{
			glm::vec4 n;
			s >> junk >> junk >> n.x >> n.y >> n.z;
			n.x = -n.x;
			normals.push_back(n);
		}
		if (line[0] == 'f')
		{
			int f[3];
			int n[3];
			s >> junk >> f[0] >> n[0] >> f[1] >> n[1] >> f[2] >> n[2];

			Triangle tri{};
			tri.vertA = verts[f[0] - 1];
			tri.vertB = verts[f[1] - 1];
			tri.vertC = verts[f[2] - 1];
			tri.normA = normals[n[0] - 1];
			tri.normB = normals[n[1] - 1];
			tri.normC = normals[n[2] - 1];
			model.triangles.push_back(tri);
		}
	}

	return model;
}



bool BuildAndDoEverythingElseWithBVH()
{
	Model testo = LoadModel("ringworld2.OBJ_MODEL");
	/*BoundingBox modelBounds{};
	for (int i = 0; i < testo.triangles.size(); i++)
	{
		const Triangle& tri = testo.triangles[i];
		glm::vec3 boundsMin = glm::min(glm::min(tri.vertA, tri.vertB), tri.vertC);
		glm::vec3 boundsMax = glm::max(glm::max(tri.vertA, tri.vertB), tri.vertC);
		modelBounds.GrowToInclude(boundsMin, boundsMax);
	}
	Model testoScaled{};
	for (int i = 0; i < testo.triangles.size(); i++)
	{
		Triangle tri = testo.triangles[i];
		Triangle scaledTri;
		glm::vec4 scale = glm::vec4(1.0f / modelBounds.Size(), 1);
		scaledTri.vertA = tri.vertA * scale + glm::vec4(0.5f, 0.5f, 0.5f, 0);
		scaledTri.vertB = tri.vertB * scale + glm::vec4(0.5f, 0.5f, 0.5f, 0);
		scaledTri.vertC = tri.vertC * scale + glm::vec4(0.5f, 0.5f, 0.5f, 0);
		scaledTri.normA = tri.normA;
		scaledTri.normB = tri.normB;
		scaledTri.normC = tri.normC;
		testoScaled.triangles.push_back(scaledTri);
	}*/
	BLAS testoBLAS{ testo, 23 };
	
	std::vector<RayTraceModel> modelsBuffer;
	modelsBuffer.push_back(RayTraceModel(testoBLAS,
		glm::vec3(1.0f, 1.0f, 1.0f),
		0.5f,
		0,
		glm::vec3(0, 0, 0),
		glm::vec3(0, 0, 0),
		glm::vec3(1, 1, 1)));

	//std::cout << "Model bounds = " << modelBounds.Size().x << ", " << modelBounds.Size().y << ", " << modelBounds.Size().z << "\n";
	
	//exit(0);

	/*std::vector<TinyTriangle> tinyTriangleBuffer;
	for (int i = 0; i < testoScaled.triangles.size(); i++)
	{
		Triangle tri = testoScaled.triangles[i];
		TinyTriangle tiny = { 0 };
		tiny.Avx_Avy |= unsigned short(tri.vertA.x * 65535.0f) << 0;
		tiny.Avx_Avy |= unsigned short(tri.vertA.y * 65535.0f) << 16;
		tiny.Bvx_Bvy |= unsigned short(tri.vertB.x * 65535.0f) << 0;
		tiny.Bvx_Bvy |= unsigned short(tri.vertB.y * 65535.0f) << 16;
		tiny.Cvx_Cvy |= unsigned short(tri.vertC.x * 65535.0f) << 0;
		tiny.Cvx_Cvy |= unsigned short(tri.vertC.y * 65535.0f) << 16;
		tiny.Avz_Bvz |= unsigned short(tri.vertA.z * 65535.0f) << 0;
		tiny.Avz_Bvz |= unsigned short(tri.vertB.z * 65535.0f) << 16;
		tiny.Cvz_Anx_Any |= unsigned short(tri.vertC.z * 65535.0f) << 0;
		tiny.Cvz_Anx_Any |= unsigned char(tri.normA.x * 0.5f) << 0;

	}*/


	CreateBufferAndCount(
		"models_buffer",
		5,
		modelsBuffer.size(),
		sizeof(RayTraceModel) * modelsBuffer.size(),
		(void*)modelsBuffer.data());

	CreateBufferAndCount(
		"triangle_buffer",
		6,
		testoBLAS.m_orderedTriangles.size(),
		sizeof(Triangle) * testoBLAS.m_orderedTriangles.size(),
		(void*)testoBLAS.m_orderedTriangles.data());

	CreateBufferAndCount(
		"nodes_buffer",
		7,
		testoBLAS.m_nodes.nodes.size(),
		sizeof(BLAS::Node) * testoBLAS.m_nodes.nodes.size(),
		(void*)testoBLAS.m_nodes.nodes.data());

	//CreateBuffer("node_buffer", 7, 0, 0);

	//std::cout << "glGetError() = " << glGetError() << "\n";

	return false;
}
