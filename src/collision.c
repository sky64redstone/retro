#include "collision.h"

rect_t rect(vec2_t pos, vec2_t size) {
  return (rect_t){ pos, size };
}

int collision_aabb(rect_t a, rect_t b) {
  return a.pos.x < b.pos.x + b.size.x &&
         a.pos.x + a.size.x > b.pos.x &&
         a.pos.y < b.pos.y + b.size.y &&
         a.pos.y + a.size.y > b.pos.y;
}
