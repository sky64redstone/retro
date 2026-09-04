#pragma once

struct vec2 {
  float x;
  float y;
};

typedef struct vec2 vec2_t;

vec2_t vec2_zero();
vec2_t vec2_splat(float x);
vec2_t vec2(float x, float y);
vec2_t vec2_rand(int max_x, int max_y);
vec2_t vec2_add(vec2_t a, vec2_t b);
vec2_t vec2_sub(vec2_t a, vec2_t b);
vec2_t vec2_scale(vec2_t v, float x);
vec2_t vec2_ceil(vec2_t v);
vec2_t vec2_floor(vec2_t v);
vec2_t vec2_round(vec2_t v);
float vec2_len(vec2_t v);
float vec2_dot(vec2_t a, vec2_t b);
int vec2_equal(vec2_t a, vec2_t b);

