#pragma once

#include "Hittable.h"
#include <glm/glm.hpp>

struct sphere_info
{
	glm::vec3 pos;
	float radious;
	int mat;
	glm::vec3 alb;
	float fuz;
};


class sphere : public hittable {
public:
	__device__  sphere() {}
	__device__  sphere(const glm::vec3 cen, float rad, material* m = 0) : center(cen), radius(rad), mat(m) {}
	__device__  virtual bool hit(const ray& r, float ray_tmin, float ray_tmax, hit_record& rec) const {
		glm::vec3 oc = center - r.origin();
		float a = dot(r.direction(), r.direction());
		float h = dot(r.direction(), oc);
		float c = glm::dot(oc, oc) - radius * radius;
		float discriminant = h * h - a * c;
		if (discriminant < 0.0f) return false;
		float root = (h - sqrt(discriminant)) / a;
		if (root <= ray_tmin || ray_tmax <= root) {
			root = (h + sqrt(discriminant)) / a;
			if (root <= ray_tmin || ray_tmax <= root) return false;
		}
		rec.t = root;
		rec.p = r.at(root);
		rec.normal = (rec.p - center) / radius;
		rec.normal = (rec.p - center) / radius;
		rec.mat = mat;
		return true;
	}
//private:
	glm::vec3 center;
	float radius;
	material* mat;
};