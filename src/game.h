#pragma once

#include <SDL3/SDL.h>

#include "vec.h"
#include "color.h"
#include "graphics.h"
#include "input.h"
#include "collision.h"

enum direction {
  UP, DOWN, LEFT, RIGHT
};

enum gamestate {
  GAME_RUNNING,
  GAME_OVER,
  GAME_PAUSED,
  GAME_WON
};

typedef struct game game_t;

struct game {
  void (*init   )(game_t* game, vec2_t win_size);
  void (*reset  )(game_t* game, vec2_t win_size);
  void (*update )(game_t* game, input_t* input, float dt);
  void (*render )(game_t* game, SDL_Renderer* renderer);
  void (*destroy)(game_t* game);
  void* data;
  enum gamestate state;
  color_t color_bg;
};

