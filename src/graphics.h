#pragma once

#include <SDL3/SDL.h>

enum alignment {
  ALIGN_LEFT,
  ALIGN_RIGHT,
  ALIGN_MIDDLE
};

void render_rect(SDL_Renderer* renderer, vec2_t pos, vec2_t size, color_t color);
void render_text(
  SDL_Renderer* renderer, vec2_t pos, vec2_t scale,
  color_t color, const char* text, enum alignment align
);

