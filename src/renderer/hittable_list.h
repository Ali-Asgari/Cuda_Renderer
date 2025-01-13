#pragma once

#include "Hittable.h"

class hittable_list : public hittable {
public:
	hittable** objects;
	int size;
	__host__ __device__ hittable_list() {}
	__host__ __device__ hittable_list(hittable** objs, int len) { objects = objs; size = len; }
	__device__ virtual bool hit(const ray& r, float ray_tmin, float ray_tmax, hit_record& rec) const {
		hit_record temp_rec;
		bool hit_anything = false;
		float closest_hit = ray_tmax;

		for (int i = 0; i < size; i++) {
			if (objects[i]->hit(r, ray_tmin, closest_hit, temp_rec)) {
				hit_anything = true;
				closest_hit = temp_rec.t;
				rec = temp_rec;
			}
		}

		return hit_anything;
	}
};
