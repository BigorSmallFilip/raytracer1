#include "bvh.hpp"

#include <glm/glm.hpp>

#include <iostream>
#include <string>

#include "program.hpp"
#include "utility.hpp"
#include "buffer.hpp"



Model LoadModel(const char* const filepath)
{
	
	
	return Model();
}



bool BuildAndDoEverythingElseWithBVH()
{
	Model testo = LoadModel("cube.OBJ_MODEL_DAMN_IT");
	
	//CreateBuffer("model_buffer", 5, 0, 0);

	//exit(0);

	CreateBufferAndCount("triangle_buffer", 6, sizeof(Triangle) * testo.triangles.size(), (void*)testo.triangles.data());

	//CreateBuffer("node_buffer", 7, 0, 0);

	//std::cout << "glGetError() = " << glGetError() << "\n";

	return false;
}
