#include "bvh.hpp"

#include <glm/glm.hpp>

#include <iostream>
#include <string>
#include <sstream>
#include <strstream>

#include "program.hpp"
#include "utility.hpp"
#include "buffer.hpp"



Model LoadModel(const char* const filepath)
{
	std::ifstream f(filepath);
	if (!f.is_open())
		return Model();

	Model model{};

	// Local cache of verts
	std::vector<float3> verts;

	while (!f.eof())
	{
		char line[128];
		f.getline(line, 128);

		std::strstream s;
		s << line;

		char junk;

		if (line[0] == 'v')
		{
			float3 v;
			s >> junk >> v.x >> v.y >> v.z;
			v.x = -v.x;
			verts.push_back(v);
		}

		if (line[0] == 'f')
		{
			int f[3];
			s >> junk >> f[0] >> f[1] >> f[2];

			Triangle tri{};
			tri.vertA = verts[f[0] - 1];
			tri.vertB = verts[f[1] - 1];
			tri.vertC = verts[f[2] - 1];
			tri.normA = float3{ 0 };
			tri.normB = float3{ 0 };
			tri.normC = float3{ 0 };
			model.triangles.push_back(tri);
		}
	}

	return model;
}



bool BuildAndDoEverythingElseWithBVH()
{
	Model testo = LoadModel("cube.OBJ_MODEL");
	
	//CreateBuffer("model_buffer", 5, 0, 0);

	//exit(0);

	CreateBufferAndCount(
		"triangle_buffer",
		6,
		testo.triangles.size(),
		sizeof(Triangle) * testo.triangles.size(),
		(void*)testo.triangles.data());

	//CreateBuffer("node_buffer", 7, 0, 0);

	//std::cout << "glGetError() = " << glGetError() << "\n";

	return false;
}
