#include <string.h>
#include <assert.h>

#include "game.h"

void render_rect(SDL_Renderer* renderer, vec2_t pos, vec2_t size, color_t color) {
  SDL_FRect rect = {
    .x = pos.x,
    .y = pos.y,
    .w = size.x,
    .h = size.y
  };

  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  SDL_RenderFillRect(renderer, &rect);
}

void render_triangle(
  SDL_Renderer* renderer, vec2_t a, vec2_t b, vec2_t c, color_t color
) {
  SDL_FColor vertex_color = {
    color.r / 255.0f,
    color.g / 255.0f,
    color.b / 255.0f,
    color.a / 255.0f
  };

  SDL_Vertex verts[] = {
    {
      .position = { a.x, a.y },
      .color = vertex_color,
      .tex_coord = { 0.0f, 0.0f }
    },
    {
      .position = { b.x, b.y },
      .color = vertex_color,
      .tex_coord = { 0.0f, 0.0f }
    },
    {
      .position = { c.x, c.y },
      .color = vertex_color,
      .tex_coord = { 0.0f, 0.0f }
    }
  };

  SDL_RenderGeometry(renderer, NULL, verts, 3, NULL, 0);
}

void draw_triangle(
  SDL_Renderer* renderer, vec2_t a, vec2_t b, vec2_t c, color_t color
) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  SDL_RenderLine(renderer, a.x, a.y, b.x, b.y);
  SDL_RenderLine(renderer, b.x, b.y, c.x, c.y);
  SDL_RenderLine(renderer, c.x, c.y, a.x, a.y);
}

void render_text(
  SDL_Renderer* renderer, vec2_t pos, vec2_t scale,
  color_t color, const char* text, enum alignment align
) {
  SDL_SetRenderScale(renderer, scale.x, scale.y);
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  switch (align) {
    case ALIGN_LEFT: {
      SDL_RenderDebugText(
        renderer,
        pos.x,
        pos.y,
        text
      );
      break;
    }
    case ALIGN_RIGHT: {
      size_t len = strlen(text);
      SDL_RenderDebugText(
        renderer,
        pos.x - len*8, /* 8px wide font */
        pos.y,
        text
      );
      break;
    }
    case ALIGN_MIDDLE: {
      size_t len = strlen(text);
      SDL_RenderDebugText(
        renderer,
        pos.x - len*4, /* 8px wide font -> middle: 8/2=4 */
        pos.y,
        text
      );
      break;
    }
    default: assert(false && "Unknown alignment"); break;
  }
  SDL_SetRenderScale(renderer, 1.f, 1.f);
}
