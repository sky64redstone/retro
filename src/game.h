#pragma once

#include <stdbool.h>
#include <SDL3/SDL.h>

#include "vec.h"
#include "color.h"

enum key {
  KEY_UP,
  KEY_DOWN,
  KEY_LEFT,
  KEY_RIGHT,
  KEY_ACTION,
  KEY_PAUSE,

  KEY_COUNT
};

struct input {
  bool down[KEY_COUNT];
  bool pressed[KEY_COUNT];
  bool released[KEY_COUNT];
};

typedef struct input input_t;
typedef struct game game_t;

struct game {
  void (*init   )(game_t* game);
  void (*reset  )(game_t* game);
  void (*update )(game_t* game, input_t* input, float dt);
  void (*render )(game_t* game, SDL_Renderer* renderer);
  void (*destroy)(game_t* game);
  void* data;
};

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

