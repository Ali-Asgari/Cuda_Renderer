#include "CudaInterop.h"

#include "device_launch_parameters.h"
#include <cuda_gl_interop.h>
#include <curand_kernel.h>
#include <iostream>
#include "glm/vec4.hpp"

#include "ray.h"
#include "sphere.h"
#include "hittable_list.h"
#include "material.h"

__global__ void create_world(hittable** objects, hittable_list** world) {
    if (threadIdx.x == 0 && blockIdx.x == 0) {
        *(objects) = new sphere(glm::vec3(0, 0, -1), 0.2, new lambertian(glm::vec3(0.8, 0.3, 0.3)));
        *(objects + 1) = new sphere(glm::vec3(1, 0, -1), 0.2, new metal(glm::vec3(0.8, 0.8, 0.8), 0.1));
        *(objects + 2) = new sphere(glm::vec3(0, -100.5, -1), 100, new metal(glm::vec3(0.8, 0.6, 0.2), 0.1));
        *world = new hittable_list(objects, 3);
    }
}

__global__ void render_init(int max_x, int max_y, curandState* rand_state) {
    int i = threadIdx.x + blockIdx.x * blockDim.x;
    int j = threadIdx.y + blockIdx.y * blockDim.y;
    if ((i >= max_x) || (j >= max_y)) return;
    int pixel_index = j * max_x + i;
    //Each thread gets same seed, a different sequence number, no offset
    curand_init(1984, pixel_index, 0, &rand_state[pixel_index]);
}



CudaInterop::CudaInterop(int width, int height)
:width(width),height(height),num_frame(1)
{
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA,GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    cudaGraphicsGLRegisterImage(&cudaResource, textureID, GL_TEXTURE_2D, cudaGraphicsRegisterFlagsWriteDiscard);

    size_t fb_size = 4 * width * height * sizeof(glm::vec4);
    cudaMalloc((void**)&fb, fb_size);
    cudaMalloc((void**)&res, fb_size);
    cudaMalloc((void**)&objects, 3 * sizeof(hittable*));
    cudaMalloc((void**)&world, sizeof(hittable_list*));
    cudaMalloc((void**)&d_rand_state, width * height * sizeof(curandState));

    cudaDeviceSynchronize();
    create_world << <1, 1 >> > (objects, world);
    cudaDeviceSynchronize();

    int nx = width;
    int ny = height;
    int tx = 8;
    int ty = 8;
    dim3 blocks(nx / tx + 1, ny / ty + 1);
    dim3 threads(tx, ty);
    render_init << <blocks, threads >> > (nx, ny, d_rand_state);
    cudaDeviceSynchronize();
}

__global__ void free_world(hittable** objects, hittable_list** world) {
    for (int i = 0;i < (*world)->size;i++)
        delete* (objects + i);
    delete* world;
}

CudaInterop::~CudaInterop()
{
    cudaGraphicsUnregisterResource(cudaResource);
    glDeleteTextures(1, &textureID);
    cudaFree(fb);
    cudaFree(res);
    free_world << <1, 1 >> > (objects, world);
    cudaFree(objects);
    cudaFree(world);
}

__device__ glm::vec3 color(const ray& r, hittable_list** world, curandState* local_rand_state) {
    ray cur_ray = r;
    glm::vec3 cur_attenuation = glm::vec3(1.0, 1.0, 1.0);
    //max depth is 50 
    for (int i = 0; i < 50; i++) {
        hit_record rec;
        if ((*world)->hit(cur_ray, 0.001f, FLT_MAX, rec)) {
            ray scattered;
            glm::vec3 attenuation;
            if (rec.mat->scatter(cur_ray, rec, attenuation, scattered, local_rand_state)) {
                cur_attenuation *= attenuation;
                cur_ray = scattered;
            }
            else {
                return glm::vec3(0.0, 0.0, 0.0);
            }
        }
        else {
            glm::vec3 unit_direction = glm::normalize(cur_ray.direction());
            float t = 0.5f * (unit_direction.y + 1.0f);
            glm::vec3 c = (1.0f - t) * glm::vec3(1.0, 1.0, 1.0) + t * glm::vec3(0.5, 0.7, 1.0);
            return cur_attenuation * c;
        }
    }
    return glm::vec3(0.0, 0.0, 0.0); // exceeded recursion
}

__global__ void render(glm::vec4* fb, int num_frame, glm::vec4* res, int max_x, int max_y, glm::vec3 lower_left_corner, glm::vec3 horizontal, glm::vec3 vertical, glm::vec3 origin, hittable_list** world, curandState* rand_state) {
    int i = threadIdx.x + blockIdx.x * blockDim.x;
    int j = threadIdx.y + blockIdx.y * blockDim.y;
    if ((i >= max_x) || (j >= max_y)) return;
    int pixel_index = j * max_x + i;
    curandState local_rand_state = rand_state[pixel_index];

    float u = float(i + curand_uniform(&local_rand_state)) / float(max_x);
    float v = float(j + curand_uniform(&local_rand_state)) / float(max_y);
    ray r(origin, lower_left_corner + u * horizontal + v * vertical);
    glm::vec4 col = glm::vec4(color(r, world, rand_state), 1.0);
    fb[pixel_index] += col;
    glm::vec4 finalcol = fb[pixel_index] / (float)num_frame;
    res[pixel_index] = finalcol;

    rand_state[pixel_index] = local_rand_state;

    //add Gamma Correction
    //finalcol[0] = sqrt(finalcol[0]);
    //finalcol[1] = sqrt(finalcol[1]);
    //finalcol[2] = sqrt(finalcol[2]);
    //res[pixel_index] = finalcol;

}

void run(int max_x, int max_y, glm::vec4* fb, int num_frame, glm::vec4* res, hittable_list** world, curandState* d_rand_state)
{
    int nx = max_x;
    int ny = max_y;
    int tx = 8;
    int ty = 8;

    dim3 blocks(nx / tx + 1, ny / ty + 1);
    dim3 threads(tx, ty);

    render << <blocks, threads >> > (fb, num_frame, res, nx, ny, glm::vec3(-2.0, -1.0, -1.0),
        glm::vec3(4.0, 0.0, 0.0),
        glm::vec3(0.0, 2.0, 0.0),
        glm::vec3(0.0, 0.0, 0.0),
        world, d_rand_state);
    cudaDeviceSynchronize();
}


void CudaInterop::renderToTexture()
{
    //if (num_frame > 500) return;
    cudaGraphicsMapResources(1, &cudaResource, 0);
    cudaArray* cudaArray;
    cudaGraphicsSubResourceGetMappedArray(&cudaArray, cudaResource, 0, 0);
    //run main kernel after mapping
    run(width, height, fb, num_frame, res, world, d_rand_state);
    num_frame += 1;
    cudaMemcpy2DToArray(cudaArray, 0, 0, res, width * 4 * sizeof(float), width * 4 * sizeof(float), height, cudaMemcpyDeviceToDevice);
    cudaGraphicsUnmapResources(1, &cudaResource, 0);
}

__global__ void recreate_cuda(hittable** objects, hittable_list** world, int size, sphere_info* d_world) {
    if (threadIdx.x == 0 && blockIdx.x == 0) {
        for (int i = 0;i < size;i++) {
            if (d_world[i].mat == 0) {
                *(objects + i) = new sphere(d_world[i].pos, d_world[i].radious, new lambertian(d_world[i].alb));
            }
            else {
                *(objects + i) = new sphere(d_world[i].pos, d_world[i].radious, new metal(d_world[i].alb, (d_world[i].fuz)));
            }
        }
        *world = new hittable_list(objects, size);
    }
}

__global__ void reset_result(int max_x, int max_y, glm::vec4* fb, glm::vec4* res) {
    int i = threadIdx.x + blockIdx.x * blockDim.x;
    int j = threadIdx.y + blockIdx.y * blockDim.y;
    int pixel_index = j * max_x + i;
    fb[pixel_index] = glm::vec4(0);
    res[pixel_index] = glm::vec4(0);
}


void CudaInterop::recreate(std::vector<sphere_info> h_world)
{

    free_world << <1, 1 >> > (objects, world);
    cudaFree(objects);
    cudaFree(world);
    cudaDeviceSynchronize();

    sphere_info* d_world_sph;
    
    cudaMalloc((void**)&objects, h_world.size() * sizeof(hittable*));
    cudaMalloc((void**)&world, sizeof(hittable_list*));
    cudaMalloc((void**)&d_world_sph, h_world.size() * sizeof(sphere_info));
    cudaDeviceSynchronize();

    cudaMemcpy(d_world_sph, h_world.data(), h_world.size() * sizeof(sphere_info), cudaMemcpyHostToDevice);
    cudaDeviceSynchronize();

    recreate_cuda << <1, 1 >> > (objects, world, h_world.size(), d_world_sph);
    cudaDeviceSynchronize();

    //reset rendering
    num_frame = 1;
    int nx = width;
    int ny = height;
    int tx = 8;
    int ty = 8;

    dim3 blocks(nx / tx + 1, ny / ty + 1);
    dim3 threads(tx, ty);

    reset_result << <blocks, threads >> > (nx, ny, fb, res);
    cudaDeviceSynchronize();

    cudaFree(d_world_sph);
    cudaDeviceSynchronize();
}
