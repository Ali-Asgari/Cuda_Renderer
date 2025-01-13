#pragma once

#include "cuda_runtime.h"
#include "glm/vec3.hpp"


class ray
{
public:
    __device__ ray() {}
    __device__ ray(const glm::vec3& origin, const glm::vec3& direction) : orig(origin), dir(direction) {}
    __device__ glm::vec3 origin() const { return orig; }
    __device__ glm::vec3 direction() const { return dir; }
    __device__ glm::vec3 at(float t) const { return orig + t * dir; }
private:
    glm::vec3 orig;
    glm::vec3 dir;
};
