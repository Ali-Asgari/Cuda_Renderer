#pragma once

#include "ray.h"

class material;

class hit_record {
public:
    glm::vec3 p;
    glm::vec3 normal;
    float t;
    material* mat;
};

class hittable {
public:
    __device__ virtual bool hit(const ray& r, float ray_tmin, float ray_tmax, hit_record& rec) const = 0;
};
