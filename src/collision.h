#pragma once

#include "vec.h"

struct rect {
  vec2_t pos;
  vec2_t size;
};

typedef struct rect rect_t;

rect_t rect(vec2_t pos, vec2_t size);
int collision_aabb(rect_t a, rect_t b);
