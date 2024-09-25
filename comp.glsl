#version 460 core



const float INFINITY = 1.0 / 0.0;
const float PI = 3.14159265359;
const float EPSILON = 0.000001;



uniform mat4 cameraToWorld;
uniform vec2 viewportScale;

layout(binding = 0, rgba32f) writeonly uniform image2D gAlbedoSpecular;
layout(binding = 1, rgba32f) writeonly uniform image2D gPosition;
layout(binding = 2, rgba32f) writeonly uniform image2D gNormal;
layout(binding = 3, r32f)    writeonly uniform image2D gDepth;



struct Ray {
	vec3 pos;
	vec3 dir;
};

struct RayHit {
    bool hit;
	vec3 pos;
	float dist;
	vec3 normal;
	vec4 albedoSpecular;
};

struct Sphere {
	vec3 position;
	float radius;
	vec4 albedoSpecular;
};

struct Triangle {
	vec3 vertA, vertB, vertC;
	vec3 normA, normB, normC;
};

struct TriangleHitInfo {
	bool hit;
	float dist;
	vec3 pos;
	vec3 normal;
	int triIndex;
};

struct RayTracingMaterial {
	vec4 albedoSpecular;
	int flag;
};

struct Model {
	int nodeOffset;
	int triOffset;
	mat4 worldToLocalMatrix;
    mat4 localToWorldMatrix;
	RayTracingMaterial material;
};

struct BVHNode {
	vec3 boundsMin;
	vec3 boundsMax;
	// index refers to triangles if is leaf node (triangleCount > 0)
	// otherwise it is the index of the first child node
	int startIndex;
	int triangleCount;
};

struct ModelHitInfo {
	bool hit;
	vec3 normal;
	vec3 pos;
	float dst;
	RayTracingMaterial material;
};



layout(binding = 4, std430) readonly buffer sphere_buffer {
    int sphereCount;
    Sphere spheres[];
};

layout(binding = 5, std430) readonly buffer model_buffer {
    int modelCount;
    Model models[];
};
layout(binding = 6, std430) readonly buffer triangle_buffer {
    int triangleCount;
    Triangle triangles[];
};
layout(binding = 7, std430) readonly buffer node_buffer {
    int nodesCount;
    BVHNode nodes[];
};





Ray create_ray(vec3 pos, vec3 dir) {
    Ray ray;
    ray.pos = pos;
    ray.dir = dir;
    return ray;
}

Ray create_camera_ray(vec2 uv) {
    // Transform the camera origin to world space
    vec3 pos = (vec4(0.0, 0.0, 0.0, 1.0) * cameraToWorld).xyz;

    // Invert the perspective projection of the view-space position
    vec3 dir = (vec4(uv * viewportScale, 1.0, 1.0) * cameraToWorld).xyz;
    dir -= pos;
    dir = normalize(dir);

    return create_ray(pos, dir);
}

RayHit create_ray_hit() {
    RayHit hit;
    hit.hit = false;
    hit.pos = vec3(0);
    hit.dist = INFINITY;
    hit.normal = vec3(0);
    hit.albedoSpecular = vec4(1.0, 0.0, 1.0, 0.0);
    return hit;
}



void ray_sphere_intersection(Ray ray, inout RayHit bestHit, Sphere sphere) {
    // Calculate distance along the ray where the sphere is intersected
    vec3 d = ray.pos - sphere.position;
    float p1 = -dot(ray.dir, d);
    float p2sqr = p1 * p1 - dot(d, d) + sphere.radius * sphere.radius;
    if (p2sqr < 0)
        return;
    float p2 = sqrt(p2sqr);
    float t = p1 - p2 > 0 ? p1 - p2 : p1 + p2;
    if (t > 0 && t < bestHit.dist) {
        bestHit.hit = true;
        bestHit.dist = t;
        bestHit.pos = ray.pos + t * ray.dir;
        bestHit.normal = normalize(bestHit.pos - sphere.position);
        bestHit.albedoSpecular = sphere.albedoSpecular;
    } else {
        bestHit.hit = false;
    }
}

TriangleHitInfo ray_triangle_intersection(Ray ray, Triangle tri)
{
	vec3 edgeAB = tri.vertB - tri.vertA;
	vec3 edgeAC = tri.vertC - tri.vertA;
	vec3 normalVector = cross(edgeAB, edgeAC);
	vec3 ao = ray.pos - tri.vertA;
	vec3 dao = cross(ao, ray.dir);

	float det = -dot(ray.dir, normalVector);
	float invDet = 1.0 / det;

	// Calculate dist to triangle & barycentric coordinates of intersection point
	float dist = dot(ao, normalVector) * invDet;
	float u = dot(edgeAC, dao) * invDet;
	float v = -dot(edgeAB, dao) * invDet;
	float w = 1 - u - v;

	// Initialize hit info
	TriangleHitInfo hitInfo;
	hitInfo.hit = det >= EPSILON && dist >= 0 && u >= 0 && v >= 0 && w >= 0;
	hitInfo.pos = ray.pos + ray.dir * dist;
	hitInfo.normal = normalize(tri.normA * w + tri.normB * u + tri.normC * v);
	hitInfo.dist = dist;
	return hitInfo;
}



RayHit trace(Ray ray) {
    RayHit bestHit = create_ray_hit();
    //IntersectGroundPlane(ray, bestHit);

    for (int i = 0; i < sphereCount; i++) {
        Sphere sphere = spheres[i];
        ray_sphere_intersection(ray, bestHit, sphere);
    }

	/*for (int i = 0; i < triangleCount; i++) {
		Triangle tri = triangles[i];
		TriangleHitInfo hitInfo = ray_triangle_intersection(ray, tri);
		if (hitInfo.dist < bestHit.dist) {
			bestHit.hit = true;
			bestHit.pos = hitInfo.pos;
			bestHit.dist = hitInfo.dist;
			bestHit.normal = hitInfo.normal;
			bestHit.albedoSpecular = vec4(1, 1, 1, 1);
		}
	}*/
    
    return bestHit;
}



layout (local_size_x = 32, local_size_y = 30, local_size_z = 1) in;
void main() {
	ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
	vec2 normalizedScreenCoord = vec2(texelCoord) / vec2(imageSize(gAlbedoSpecular));
    vec2 uv = normalizedScreenCoord * 2 - 1;

    Ray ray = create_camera_ray(uv);
    RayHit rayhit = trace(ray);

	vec3 albedo = rayhit.albedoSpecular.rgb;
	float specular = rayhit.albedoSpecular.a;
	vec3 position = rayhit.pos;
	vec3 normal = rayhit.normal;
	float depth = rayhit.dist;

    //albedo = vec3(0);
    //albedo = vec3(triangleCount);

	imageStore(gAlbedoSpecular, texelCoord, vec4(albedo, specular));
	imageStore(gPosition, texelCoord, vec4(position, 0));
	imageStore(gNormal, texelCoord, vec4(normal, 0));
	imageStore(gDepth, texelCoord, vec4(depth, 0, 0, 0));
}

