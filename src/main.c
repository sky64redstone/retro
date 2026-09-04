#include <SDL3/SDL.h>
#include <stdbool.h>
#include <string.h>

#include "games/snake.h"

static enum key key_from_scancode(SDL_Scancode scancode) {
  switch (scancode) {
    case SDL_SCANCODE_W:
    case SDL_SCANCODE_UP:
      return KEY_UP;

    case SDL_SCANCODE_S:
    case SDL_SCANCODE_DOWN:
      return KEY_DOWN;

    case SDL_SCANCODE_A:
    case SDL_SCANCODE_LEFT:
      return KEY_LEFT;

    case SDL_SCANCODE_D:
    case SDL_SCANCODE_RIGHT:
      return KEY_RIGHT;

    case SDL_SCANCODE_SPACE:
    case SDL_SCANCODE_RETURN:
      return KEY_ACTION;

    case SDL_SCANCODE_ESCAPE:
      return KEY_PAUSE;

    default:
      return KEY_COUNT;
  }
}


int main(int argc, char** argv) {
  SDL_Window* window = NULL;
  SDL_Renderer* renderer = NULL;
  SDL_Event event;
  game_t snake;
  input_t input;
  bool running = true;

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return 1;
  }

  if (!SDL_CreateWindowAndRenderer(
    "Retro Games",
    800,
    640,
    0,
    &window,
    &renderer
  )) {
    SDL_Log("CreateWindowAndRenderer failed: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  if (!SDL_SetRenderVSync(renderer, 1)) {
    SDL_Log("Failed to enable VSync: %s", SDL_GetError());
  }

  memcpy(&snake, &snake_template, sizeof(game_t));
  memset(&input, 0, sizeof(input_t));

  snake.init(&snake);

  Uint64 previous_time = SDL_GetTicksNS();

  while (running) {
    memset(&input.pressed, false, KEY_COUNT * sizeof(bool));
    memset(&input.released, false, KEY_COUNT * sizeof(bool));
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_EVENT_QUIT: {
          running = false;
          break;
        }
        case SDL_EVENT_KEY_DOWN: {
          enum key k = key_from_scancode(event.key.scancode);

          if (k != KEY_COUNT) {
            if (!event.key.repeat && !input.down[k]) {
              input.pressed[k] = true;
            }
            input.down[k] = true;
          }
          break;
        }
        case SDL_EVENT_KEY_UP: {
          enum key k = key_from_scancode(event.key.scancode);

          if (k != KEY_COUNT) {
            input.down[k] = false;
            input.released[k] = true;
          }
          break;
        }
      }
    }

    Uint64 current_time = SDL_GetTicksNS();
    float delta_time = (float)(current_time - previous_time) / 1000000000.0f;
    previous_time = current_time;

    /* Prevent huge jumps after pausing, resizing, or debugging */
    if (delta_time > 0.1f) {
      delta_time = 0.1f;
    }

    snake.update(&snake, &input, delta_time);

    SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
    SDL_RenderClear(renderer);

    snake.render(&snake, renderer);

    SDL_RenderPresent(renderer);
  }

  snake.destroy(&snake);

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
