#include <stdlib.h>
#include <math.h>

#include "vec.h"

vec2_t vec2_zero() {
  return (vec2_t){ 0, 0 };
}

vec2_t vec2_splat(float x) {
  return (vec2_t){ x, x };
}

vec2_t vec2(float x, float y) {
  return (vec2_t){ x, y };
}

vec2_t vec2_rand(int max_x, int max_y) {
  return (vec2_t){ rand() % max_x, rand() % max_y };
}

vec2_t vec2_add(vec2_t a, vec2_t b) {
  return (vec2_t){ a.x + b.x, a.y + b.y };
}

vec2_t vec2_sub(vec2_t a, vec2_t b) {
  return (vec2_t){ a.x - b.x, a.y - b.y };
}

vec2_t vec2_scale(vec2_t v, float x) {
  return (vec2_t){ v.x * x, v.y * x };
}

vec2_t vec2_ceil(vec2_t v) {
  return (vec2_t){ ceilf(v.x), ceilf(v.y) };
}

vec2_t vec2_floor(vec2_t v) {
  return (vec2_t){ floorf(v.x), floorf(v.y) };
}

vec2_t vec2_round(vec2_t v) {
  return (vec2_t){ roundf(v.x), roundf(v.y) };
}

vec2_t vec2_norm(vec2_t v) {
  float len = vec2_len(v);
  return (vec2_t){ v.x / len, v.y / len };
}

float vec2_len(vec2_t v) {
  return hypotf(v.x, v.y);
}

float vec2_dot(vec2_t a, vec2_t b) {
  return a.x * b.x + a.y * b.y;
}

int vec2_equal(vec2_t a, vec2_t b) {
  return a.x == b.x && a.y == b.y;
}

