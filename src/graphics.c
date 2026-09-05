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
