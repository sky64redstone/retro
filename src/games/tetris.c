#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <time.h>

#include "game.h"
#include "tetris-pieces.h"

#define WIDTH 10
#define HEIGHT 20
#define SCALE 32
#define SPEED_SEC 0.3f

static void tetris_init(game_t* game, vec2_t win_size);
static void tetris_reset(game_t* game, vec2_t win_size);
static void tetris_update(game_t* game, input_t* input, float dt);
static void tetris_render(game_t* game, SDL_Renderer* renderer);
static void tetris_destroy(game_t* game);

const game_t tetris_template = {
  .init = tetris_init,
  .reset = tetris_reset,
  .update = tetris_update,
  .render = tetris_render,
  .destroy = tetris_destroy,
  .data = NULL,
  .state = GAME_RUNNING,
  .color_bg = { 30, 30, 40, 255 }
};

static const color_t color_field = { 0, 0, 0, 255 };
static const color_t color_white = { 255, 255, 255, 255 };

typedef struct tetris_active {
  const tetris_piece_t* type;
  vec2_t pos;
  int rotation;
} tetris_active_t;

struct tetris_data {
  int field[WIDTH * HEIGHT];
  tetris_active_t active;
  float accumulator;
};

typedef struct tetris_data tetris_data_t;

static void tetris_data_init(tetris_data_t* data);
static bool tetris_can_place(
  const tetris_data_t* data,
  const tetris_piece_t* type,
  int rotation,
  vec2_t pos
);
static bool tetris_can_move(
  const tetris_data_t* data,
  int dx,
  int dy
);
static bool tetris_try_rotate(tetris_data_t* data);
static void tetris_place_active(tetris_data_t* data);
static void tetris_check_lines(tetris_data_t* data);
static void tetris_remove_line(tetris_data_t* data, int y);
static void tetris_new_piece(tetris_data_t* data);
static int tetris_piece_index(const tetris_piece_t* piece);
static void tetris_render_piece(
  SDL_Renderer* renderer,
  const tetris_piece_t* type,
  int rotation,
  vec2_t pos
);

static void tetris_init(game_t* game, vec2_t win_size) {
  assert(game);
  assert(!game->data);

  (void)win_size;

  tetris_data_t* data = calloc(1, sizeof(tetris_data_t));
  assert(data);
  game->data = data;
  game->state = GAME_RUNNING;

  srand((unsigned int)time(NULL));
  tetris_data_init(data);

  if (!tetris_can_place(
    data,
    data->active.type,
    data->active.rotation,
    data->active.pos
  )) {
    game->state = GAME_OVER;
  }
}

static void tetris_reset(game_t* game, vec2_t win_size) {
  assert(game);
  assert(game->data);

  (void)win_size;

  memset(game->data, 0, sizeof(tetris_data_t));
  game->state = GAME_RUNNING;

  tetris_data_t* data = (tetris_data_t*)game->data;
  tetris_data_init(data);

  if (!tetris_can_place(data, data->active.type,
    data->active.rotation, data->active.pos)) {
    game->state = GAME_OVER;
  }
}

static void tetris_update(game_t* game, input_t* input, float dt) {
  assert(game);
  assert(game->data);
  assert(input);

  tetris_data_t* data = (tetris_data_t*)game->data;

  switch (game->state) {
    case GAME_RUNNING: {
      if (input->pressed[KEY_LEFT] && tetris_can_move(data, -1, 0)) {
        data->active.pos.x -= 1.0f;
      }
      if (input->pressed[KEY_RIGHT] && tetris_can_move(data, 1, 0)) {
        data->active.pos.x += 1.0f;
      }
      if (input->pressed[KEY_ACTION]) {
        tetris_try_rotate(data);
      }
      if (input->pressed[KEY_DOWN] && tetris_can_move(data, 0, 1)) {
        data->active.pos.y += 1.0f;
        data->accumulator = 0.0f;
      }
      if (input->pressed[KEY_PAUSE]) {
        game->state = GAME_PAUSED;
      }

      data->accumulator += dt;
      while (data->accumulator >= SPEED_SEC) {
        data->accumulator -= SPEED_SEC;

        if (tetris_can_move(data, 0, 1)) {
          data->active.pos.y += 1.0f;
          continue;
        }

        tetris_place_active(data);
        tetris_check_lines(data);
        tetris_new_piece(data);

        if (!tetris_can_place(
          data,
          data->active.type,
          data->active.rotation,
          data->active.pos
        )) {
          game->state = GAME_OVER;
          break;
        }
      }
      break;
    }
    case GAME_PAUSED: {
      if (input->pressed[KEY_PAUSE]) {
        game->state = GAME_RUNNING;
      }
      break;
    }
    case GAME_WON:
    case GAME_OVER: {
      if (input->pressed[KEY_ACTION]) {
        game->reset(game, vec2_zero());
      }
      break;
    }
  } /* switch (game->state) */
}

static void tetris_render(game_t* game, SDL_Renderer* renderer) {
  assert(game);
  assert(game->data);
  assert(renderer);

  tetris_data_t* data = (tetris_data_t*)game->data;

  render_rect(
    renderer,
    vec2_zero(),
    vec2_scale(vec2(WIDTH, HEIGHT), SCALE),
    color_field
  );

  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      const int piece = data->field[x + y * WIDTH];

      if (piece <= 0 || piece >= (int)(sizeof(pieces) / sizeof(pieces[0]))) {
        continue;
      }

      render_rect(
        renderer,
        vec2_scale(vec2(x, y), SCALE),
        vec2_splat(SCALE),
        pieces[piece].color
      );
    }
  }

  switch (game->state) {
    case GAME_RUNNING: {
      tetris_render_piece(
        renderer,
        data->active.type,
        data->active.rotation,
        data->active.pos
      );
      break;
    }
    case GAME_PAUSED: {
      vec2_t pos = vec2_scale(vec2(WIDTH * SCALE, 0), 0.5f);

      render_text(
        renderer,
        vec2_add(vec2_scale(pos, 0.5f), vec2_splat(5)),
        vec2_splat(4.f),
        color_white,
        "PAUSED",
        ALIGN_LEFT
      );
      render_text(
        renderer,
        vec2(
          pos.x + 10,
          pos.y + 50
        ),
        vec2_splat(2.f),
        color_white,
        "PRESS <PAUSE> TO CONTINUE",
        ALIGN_LEFT
      );
      break;
    }
    case GAME_OVER: {
      vec2_t pos = vec2_scale(vec2(WIDTH * SCALE, 0), 0.5f);

      render_text(
        renderer,
        vec2_add(vec2_scale(pos, 0.5f), vec2_splat(5)),
        vec2_splat(4.f),
        color_white,
        "GAME OVER",
        ALIGN_LEFT
      );
      render_text(
        renderer,
        vec2(
          pos.x + 10,
          pos.y + 50
        ),
        vec2_splat(2.f),
        color_white,
        "PRESS <ACTION> TO PLAY AGAIN",
        ALIGN_LEFT
      );
      break;
    }
    case GAME_WON: {
      vec2_t pos = vec2_scale(vec2(WIDTH * SCALE, 0), 0.5f);

      render_text(
        renderer,
        vec2_add(vec2_scale(pos, 0.5f), vec2_splat(5)),
        vec2_splat(4.f),
        color_white,
        "YOU WON",
        ALIGN_LEFT
      );
      render_text(
        renderer,
        vec2(
          pos.x + 10,
          pos.y + 50
        ),
        vec2_splat(2.f),
        color_white,
        "PRESS <ACTION> TO PLAY AGAIN",
        ALIGN_LEFT
      );
      break;
    }
  }
}

static void tetris_destroy(game_t* game) {
  assert(game);
  assert(game->data);

  free(game->data);
  game->data = NULL;
}

static void tetris_data_init(tetris_data_t* data) {
  tetris_new_piece(data);
}

static bool tetris_can_place(
  const tetris_data_t* data,
  const tetris_piece_t* type,
  int rotation,
  vec2_t pos
) {
  assert(data);
  assert(type);
  assert(rotation >= 0 && rotation < 4);

  for (int i = 0; i < 16; i++) {
    if (!type->layout[rotation][i]) {
      continue;
    }

    const int local_x = i % 4;
    const int local_y = i / 4;
    const int global_x = (int)pos.x + local_x;
    const int global_y = (int)pos.y + local_y;

    if (global_x < 0 || global_x >= WIDTH ||
      global_y < 0 || global_y >= HEIGHT) {
      return false;
      }

      if (data->field[global_x + global_y * WIDTH] != 0) {
        return false;
      }
  }

  return true;
}

static bool tetris_can_move(
  const tetris_data_t* data,
  int dx,
  int dy
) {
  return tetris_can_place(
    data,
    data->active.type,
    data->active.rotation,
    vec2(data->active.pos.x + dx, data->active.pos.y + dy)
  );
}

static bool tetris_try_rotate(tetris_data_t* data) {
  const int rotation = (data->active.rotation + 1) % 4;
  const vec2_t pos = data->active.pos;

  if (tetris_can_place(data, data->active.type, rotation, pos)) {
    data->active.rotation = rotation;
    return true;
  }

  if (tetris_can_place(
    data,
    data->active.type,
    rotation,
    vec2(pos.x - 1.0f, pos.y)
  )) {
    data->active.pos.x -= 1.0f;
    data->active.rotation = rotation;
    return true;
  }

  if (tetris_can_place(
    data,
    data->active.type,
    rotation,
    vec2(pos.x + 1.0f, pos.y)
  )) {
    data->active.pos.x += 1.0f;
    data->active.rotation = rotation;
    return true;
  }

  return false;
}

static void tetris_place_active(tetris_data_t* data) {
  const tetris_piece_t* type = data->active.type;
  const int rotation = data->active.rotation;
  const int itype = tetris_piece_index(type);

  assert(itype > 0);
  assert(tetris_can_place(data, type, rotation, data->active.pos));

  for (int i = 0; i < 16; i++) {
    if (!type->layout[rotation][i]) {
      continue;
    }

    const int local_x = i % 4;
    const int local_y = i / 4;
    const int global_x = (int)data->active.pos.x + local_x;
    const int global_y = (int)data->active.pos.y + local_y;

    data->field[global_x + global_y * WIDTH] = itype;
  }
}

static void tetris_check_lines(tetris_data_t* data) {
  for (int y = HEIGHT - 1; y >= 0; y--) {
    bool full = true;

    for (int x = 0; x < WIDTH; x++) {
      if (data->field[x + y * WIDTH] == 0) {
        full = false;
        break;
      }
    }

    if (full) {
      tetris_remove_line(data, y);
      y++;
    }
  }
}

static void tetris_remove_line(tetris_data_t* data, int y) {
  assert(data);
  assert(y >= 0 && y < HEIGHT);

  const size_t line = WIDTH * sizeof(data->field[0]);
  const size_t lines_above = (size_t)y * line;

  if (lines_above > 0) {
    memmove(
      &data->field[WIDTH],
      &data->field[0],
      lines_above
    );
  }

  memset(&data->field[0], 0, line);
}

static void tetris_new_piece(tetris_data_t* data) {
  const int piece_count = (int)(sizeof(pieces) / sizeof(pieces[0])) - 1;

  assert(piece_count > 0);

  data->active.type = &pieces[1 + rand() % piece_count];
  data->active.rotation = 0;
  data->active.pos = vec2((WIDTH - 4) / 2, 0);
  data->accumulator = 0.0f;
}

static int tetris_piece_index(const tetris_piece_t* piece) {
  assert(piece >= pieces);
  assert(piece < pieces + sizeof(pieces) / sizeof(pieces[0]));
  return (int)(piece - pieces);
}

static void tetris_render_piece(
  SDL_Renderer* renderer,
  const tetris_piece_t* type,
  int rotation,
  vec2_t pos
) {
  for (int i = 0; i < 16; i++) {
    if (!type->layout[rotation][i]) {
      continue;
    }

    const int local_x = i % 4;
    const int local_y = i / 4;
    const int global_x = (int)pos.x + local_x;
    const int global_y = (int)pos.y + local_y;

    if (global_x < 0 || global_x >= WIDTH ||
      global_y < 0 || global_y >= HEIGHT) {
      continue;
      }

      render_rect(
        renderer,
        vec2_scale(vec2(global_x, global_y), SCALE),
                  vec2_splat(SCALE),
                  type->color
      );
  }
}
