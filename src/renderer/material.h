#pragma once

struct hit_record;

#include <curand_kernel.h>

#include "ray.h"
#include "hittable.h"
#include <glm/glm.hpp>

#define RANDVEC3 glm::vec3(curand_uniform(local_rand_state),curand_uniform(local_rand_state),curand_uniform(local_rand_state))


__device__ glm::vec3 random_in_unit_sphere(curandState* local_rand_state) {
    glm::vec3 p;
    do {
        p = 2.0f * RANDVEC3 - glm::vec3(1, 1, 1);
    } while (glm::dot(p,p) >= 1.0f);
    return p;
}

__device__ glm::vec3 reflect(const glm::vec3& v, const glm::vec3& n) {
    return v - 2.0f * glm::dot(v, n) * n;
}


class material {
public:
    __device__ virtual bool scatter(const ray& r_in, const hit_record& rec, glm::vec3& attenuation, ray& scattered, curandState* local_rand_state) const = 0;
};


class lambertian : public material {
public:
    __device__ lambertian(const glm::vec3& albedo) : albedo(albedo) {}

    __device__ virtual bool scatter(const ray& r_in, const hit_record& rec, glm::vec3& attenuation, ray& scattered, curandState* local_rand_state) const {
        auto scatter_direction = rec.normal + random_in_unit_sphere(local_rand_state);
        scattered = ray(rec.p, scatter_direction);
        attenuation = albedo;
        return true;
    }

//private:
    glm::vec3 albedo;
};

class metal : public material {
public:
    __device__ metal(const glm::vec3& a, float f) : albedo(a) { if (f < 1) fuzz = f; else fuzz = 1; }
    __device__ virtual bool scatter(const ray& r_in, const hit_record& rec, glm::vec3& attenuation, ray& scattered, curandState* local_rand_state) const {
        glm::vec3 reflected = reflect(glm::normalize(r_in.direction()), rec.normal);
        scattered = ray(rec.p, reflected + fuzz * random_in_unit_sphere(local_rand_state));
        attenuation = albedo;
        return (glm::dot(scattered.direction(), rec.normal) > 0.0f);
    }
    glm::vec3 albedo;
    float fuzz;
};