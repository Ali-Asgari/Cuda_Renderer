#pragma once

#include <glad/glad.h>
#include "cuda_runtime.h"
#include <curand_kernel.h>
#include <cuda_gl_interop.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include "ray.h"

#include "hittable_list.h"
#include "sphere.h"
#include <vector>

class CudaInterop {
public:
	int num_frame;
	CudaInterop(int width, int height);
	~CudaInterop();;
	void renderToTexture();
	inline void bindTexture() const { glBindTexture(GL_TEXTURE_2D, textureID);}
	void recreate(std::vector<sphere_info> h_world);
private:
	GLuint textureID;
	cudaGraphicsResource_t cudaResource;
	int width, height;
	glm::vec4* fb;
	glm::vec4* res;
	hittable** objects;
	hittable_list** world;
	curandState* d_rand_state;
};

//__global__ void reset_result(int max_x, int max_y, glm::vec4* fb, glm::vec4* res);
//__global__ void render_init(int max_x, int max_y, curandState* rand_state);
//__global__ void create_world(hittable** objects, hittable_list** world);
//__global__ void free_world(hittable** objects, hittable_list** world);
//void run(int max_x, int max_y, glm::vec4* fb, int num_frame, glm::vec4* res, hittable_list** world, curandState* d_rand_state);
