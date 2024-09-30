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

uniform sampler2D testTexture;



struct Ray {
	vec3 pos;
	vec3 dir;
    vec3 invdir;
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

struct Capsule {
    vec3 posa;
    vec3 posb;
    float radius;
    vec4 albedoSpecular;
};



struct Triangle {
	vec3 vertA, vertB, vertC;
	vec3 normA, normB, normC;
};

struct BVHNode {
	vec3 boundsMin; float _padding0;
	vec3 boundsMax; float _padding1;
	// index refers to triangles if is leaf node (triangleCount > 0)
	// otherwise it is the index of the first child node
	int startIndex;
	int triangleCount;
    int _padding2;
    int _padding3;
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

struct TriangleHitInfo {
    bool hit;
	vec3 pos;
	float dist;
	vec3 normal;
    int triIndex;
};

struct ModelHitInfo {
	bool hit;
	vec3 pos;
	float dist;
	vec3 normal;
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



/*const int triangleCount = 2;
const Triangle triangles[2] = {
    { vec3(0, -5, 0), vec3(0, 0, 5), vec3(-5, -5, 0) , vec3(0, 1, 0), vec3(0, 1, 0), vec3(0, 1, 0) },
    { vec3(0, 0, 0), vec3(0, 0, 5), vec3(5, 0, 0) , vec3(0, 1, 0), vec3(0, 1, 0), vec3(0, 1, 0) },
};*/






Ray create_ray(vec3 pos, vec3 dir) {
    Ray ray;
    ray.pos = pos;
    ray.dir = dir;
    ray.invdir = 1.0 / dir;
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
    hit.albedoSpecular = vec4(0.0, 0.0, 0.0, 0.0);
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

float ray_capsule_intersection_dist(Ray ray, Capsule capsule)
{
    vec3 ba = capsule.posb - capsule.posa;
    vec3 oa = ray.pos - capsule.posa;

    float baba = dot(ba,ba);
    float bard = dot(ba,ray.dir);
    float baoa = dot(ba,oa);
    float rdoa = dot(ray.dir,oa);
    float oaoa = dot(oa,oa);

    float a = baba      - bard*bard;
    float b = baba*rdoa - baoa*bard;
    float c = baba*oaoa - baoa*baoa - capsule.radius*capsule.radius*baba;
    float h = b*b - a*c;
    if (h >= 0.0)
    {
        float t = (-b - sqrt(h)) / a;
        float y = baoa + t * bard;
        // body
        if (y > 0.0 && y < baba) return t;
        // caps
        vec3 oc = (y <= 0.0) ? oa : ray.pos - capsule.posb;
        b = dot(ray.dir, oc);
        c = dot(oc, oc) - capsule.radius * capsule.radius;
        h = b*b - c;
        if (h > 0.0) return -b - sqrt(h);
    }
    return INFINITY;
}

// compute normal
vec3 ray_capsule_intersection_normal(vec3 pos, Capsule capsule)
{
    vec3  ba = capsule.posb - capsule.posa;
    vec3  pa = pos - capsule.posa;
    float h = clamp(dot(pa,ba)/dot(ba,ba),0.0,1.0);
    return (pa - h*ba)/capsule.radius;
}

RayHit ray_capsule_intersection(Ray ray, Capsule capsule)
{
    RayHit hit = create_ray_hit();

    float dist = ray_capsule_intersection_dist(ray, capsule);
    if (dist < INFINITY) {
        hit.hit = true;
        hit.pos = ray.pos + ray.dir * dist;
        hit.normal = ray_capsule_intersection_normal(hit.pos, capsule);
        hit.dist = dist;
        hit.albedoSpecular = capsule.albedoSpecular;
    }

    return hit;
}

/*TriangleHitInfo ray_triangle_intersection2(Ray ray, Triangle tri)
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
}*/


TriangleHitInfo ray_triangle_intersection(Ray ray, Triangle tri)
{
    TriangleHitInfo hitInfo;
    hitInfo.hit = false;
    hitInfo.pos = vec3(0);
    hitInfo.dist = INFINITY;
    hitInfo.normal = vec3(0);
    
    vec3 edge1 = tri.vertB - tri.vertA;
    vec3 edge2 = tri.vertC - tri.vertA;
    vec3 ray_cross_e2 = cross(ray.dir, edge2);
    float det = dot(edge1, ray_cross_e2);

    if (det > -EPSILON && det < EPSILON)
        return hitInfo;    // This ray is parallel to this triangle.

    float inv_det = 1.0 / det;
    vec3 s = ray.pos - tri.vertA;
    float u = inv_det * dot(s, ray_cross_e2);

    if (u < 0 || u > 1)
        return hitInfo;

    vec3 s_cross_e1 = cross(s, edge1);
    float v = inv_det * dot(ray.dir, s_cross_e1);

    if (v < 0 || u + v > 1)
        return hitInfo;

    // At this stage we can compute t to find out where the intersection point is on the line.
    float t = inv_det * dot(edge2, s_cross_e1);

    if (t > EPSILON) // ray intersection
    {
        hitInfo.hit = true;
	    hitInfo.pos = ray.pos + ray.dir * t;
		float w = 1 - u - v;
	    hitInfo.normal = normalize(tri.normA * w + tri.normB * u + tri.normC * v);
        //hitInfo.normal = normalize(vec3(1, 0, 0) * w + vec3(0, 1, 0) * u + vec3(0, 0, 1) * v);
        //hitInfo.normal = normalize(cross(edge2, edge1));
	    hitInfo.dist = t;


        return hitInfo;
    }
    else // This means that there is a line intersection but not a ray intersection.
        return hitInfo;
}

// Thanks to https://tavianator.com/2011/ray_box.html
float ray_boundingbox_dist(Ray ray, vec3 boxMin, vec3 boxMax)
{
	vec3 tMin = (boxMin - ray.pos) * ray.invdir;
	vec3 tMax = (boxMax - ray.pos) * ray.invdir;
	vec3 t1 = min(tMin, tMax);
	vec3 t2 = max(tMin, tMax);
	float tNear = max(max(t1.x, t1.y), t1.z);
	float tFar = min(min(t2.x, t2.y), t2.z);

	bool hit = tFar >= tNear && tFar > 0;
	float dist = hit ? tNear > 0 ? tNear : 0 : INFINITY;
	return dist;
};



TriangleHitInfo RayTriangleBVH(Ray ray, float rayLength, int nodeOffset, int triOffset, inout ivec2 stats)
{
	TriangleHitInfo result;
	result.dist = rayLength;
	result.triIndex = -1;

	int stack[32];
	int stackIndex = 0;
	stack[stackIndex++] = nodeOffset + 0;

	while (stackIndex > 0)
	{
		BVHNode node = nodes[stack[--stackIndex]];
		bool isLeaf = node.triangleCount > 0;

		if (isLeaf)
		{
			for (int i = 0; i < node.triangleCount; i++)
			{
				Triangle tri = triangles[triOffset + node.startIndex + i];
				TriangleHitInfo triHitInfo = ray_triangle_intersection(ray, tri);
				stats[0]++; // count triangle intersection tests

				if (triHitInfo.hit && triHitInfo.dist < result.dist)
				{
					result = triHitInfo;
					result.triIndex = node.startIndex + i;
				}
			}
		}
		else
		{
			int childIndexA = nodeOffset + node.startIndex + 0;
			int childIndexB = nodeOffset + node.startIndex + 1;
			BVHNode childA = nodes[childIndexA];
			BVHNode childB = nodes[childIndexB];

			float distA = ray_boundingbox_dist(ray, childA.boundsMin, childA.boundsMax);
			float distB = ray_boundingbox_dist(ray, childB.boundsMin, childB.boundsMax);
			stats[1] += 2; // count bounding box intersection tests
						
			// We want to look at closest child node first, so push it last
			bool isNearestA = distA <= distB;
			float distNear = isNearestA ? distA : distB;
			float distFar = isNearestA ? distB : distA;
			int childIndexNear = isNearestA ? childIndexA : childIndexB;
			int childIndexFar = isNearestA ? childIndexB : childIndexA;

			if (distFar < result.dist) stack[stackIndex++] = childIndexFar;
			if (distNear < result.dist) stack[stackIndex++] = childIndexNear;
		}
	}

	return result;
}

ModelHitInfo CalculateRayCollision(Ray worldRay, out ivec2 stats)
{
	ModelHitInfo result;
	result.dist = INFINITY;
	Ray localRay;

	for (int i = 0; i < modelCount; i++)
	{
		Model model = models[i];
		// Transform ray into model's local coordinate space
		localRay.pos = vec3(vec4(worldRay.pos, 1) * model.worldToLocalMatrix);
		localRay.dir = vec3(vec4(worldRay.dir, 0) * model.worldToLocalMatrix);
		localRay.invdir = 1.0 / localRay.dir;

		// Traverse bvh to find closest triangle intersection with current model
		TriangleHitInfo hit = RayTriangleBVH(localRay, result.dist, model.nodeOffset, model.triOffset, stats);

		// Record closest hit
		if (hit.dist < result.dist)
		{
			result.hit = true;
			result.dist = hit.dist;
			result.normal = normalize(vec3(vec4(hit.normal, 0) * model.localToWorldMatrix));
			result.pos = worldRay.pos + worldRay.dir * hit.dist;
			result.material = model.material;
		}
	}

	//result.hit = true;
	//result.dist = 20;
	//result.material.albedoSpecular = vec4(0, 1, 0, 1);

	return result;
}




RayHit trace(Ray ray) {
    RayHit bestHit = create_ray_hit();
    //IntersectGroundPlane(ray, bestHit);

    for (int i = 0; i < sphereCount; i++) {
        Sphere sphere = spheres[i];
        ray_sphere_intersection(ray, bestHit, sphere);
    }

	ivec2 stats = ivec2(0);
	for (int i = 0; i < modelCount; i++) {
        ModelHitInfo modelHit = CalculateRayCollision(ray, stats);
		if (modelHit.dist < bestHit.dist) {
			bestHit.pos = modelHit.pos;
			bestHit.normal = modelHit.normal;
			bestHit.dist = modelHit.dist;
			bestHit.albedoSpecular = modelHit.material.albedoSpecular;

			float angle = atan(bestHit.pos.y / bestHit.pos.x) * 2800;
			bestHit.albedoSpecular = vec4(texture(testTexture, vec2(bestHit.pos.z, angle) * 0.2).rgb, 0.5);

			/*const int boxMax = 200;
			const int triMax = 20;
			bestHit.albedoSpecular = vec4(float(stats.x) / triMax, 0, float(stats.y) / boxMax, 1);
			if (stats.x > triMax) bestHit.albedoSpecular = vec4(1, 0.75, 0.75, 1);
			if (stats.y > boxMax) bestHit.albedoSpecular = vec4(0.75, 0.75, 1, 1);
			if (stats.y > boxMax && stats.x > triMax) bestHit.albedoSpecular = vec4(0.25, 0, 0, 1);*/

		}
    }

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

	//albedo = texture(testTexture, uv).rgb;
	//albedo = imageLoad(testTexture, texelCoord).rgb;
	
    //albedo = vec3(uv / 2 + 0.5, 0);
    
	imageStore(gAlbedoSpecular, texelCoord, vec4(albedo, specular));
	imageStore(gPosition, texelCoord, vec4(position, 0));
	imageStore(gNormal, texelCoord, vec4(normal, 0));
	imageStore(gDepth, texelCoord, vec4(depth, 0, 0, 0));
}

