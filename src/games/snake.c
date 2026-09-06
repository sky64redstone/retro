#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include <stdio.h>

#include "snake.h"
#include "vec.h"

#define BOARD_WIDTH 25
#define BOARD_HEIGHT 20
#define BOARD_SCALE 32
#define MAX_LENGTH (BOARD_WIDTH * BOARD_HEIGHT)
#define SPEED_SEC 0.3f

static void snake_init(game_t* game, vec2_t win_size);
static void snake_reset(game_t* game, vec2_t win_size);
static void snake_update(game_t* game, input_t* input, float dt);
static void snake_render(game_t* game, SDL_Renderer* renderer);
static void snake_destroy(game_t* game);

const struct game snake_template = {
  .init = snake_init,
  .reset = snake_reset,
  .update = snake_update,
  .render = snake_render,
  .destroy = snake_destroy,
  .data = NULL,
  .state = GAME_PAUSED,
  .color_bg = { 30, 30, 40, 255 }
};

struct snake_data {
  struct {
    vec2_t body[MAX_LENGTH];
    size_t length;
    enum direction dir;
    enum direction next;
  } snake;
  vec2_t food_pos;
  float accumulator;
  vec2_t win_size;
};

typedef struct snake_data snake_data_t;

static color_t color_body = { 200, 200, 200, 255 };
static color_t color_head = { 230, 230, 230, 255 };
static color_t color_food = { 255, 0, 0, 255 };
static color_t color_board1 = { 50, 150, 50, 255 };
static color_t color_board2 = { 50, 140, 50, 255 };
static color_t color_menu = { 30, 30, 40, 255 };
static color_t color_white = { 255, 255, 255, 255 };

/* internal functions */
static void snake_data_init(snake_data_t* data, vec2_t win_size);
static void snake_new_food(snake_data_t* data);
static void snake_update_dir(snake_data_t* data, input_t* input);
static void snake_update_body(game_t* game);
static int snake_contains(snake_data_t* data, vec2_t pos);

static void snake_init(game_t* game, vec2_t win_size) {
  assert(game);
  assert(!game->data);

  snake_data_t* data = calloc(1, sizeof(snake_data_t));
  game->data = data;

  snake_data_init(data, win_size);

  srand(time(NULL));
}

static void snake_reset(game_t* game, vec2_t win_size) {
  assert(game);
  assert(game->data);

  memset(game->data, 0, sizeof(snake_data_t));
  snake_data_init((snake_data_t*)game->data, win_size);
}

static void snake_update(game_t* game, input_t* input, float dt) {
  assert(game);
  assert(game->data);

  snake_data_t* data = (snake_data_t*)game->data;

  switch (game->state) {
    case GAME_RUNNING: {
      data->accumulator += dt;

      snake_update_dir(data, input);
      
      if (input->pressed[KEY_PAUSE]) {
        game->state = GAME_PAUSED;
        break;
      }

      if (data->accumulator >= SPEED_SEC) {
        data->snake.dir = data->snake.next;

        snake_update_body(game);

        if (vec2_equal(data->snake.body[0], data->food_pos)) {
          snake_new_food(data);
          data->snake.body[data->snake.length] =
            data->snake.body[data->snake.length - 1];
          data->snake.length++;
        }

        data->accumulator -= SPEED_SEC;
      }
      break;
    }
    case GAME_OVER:
    case GAME_WON: {
      if (input->pressed[KEY_ACTION]) {
        game->reset(game, data->win_size);
      }
      break;
    }
    case GAME_PAUSED: {
      if (input->pressed[KEY_PAUSE]) {
        game->state = GAME_RUNNING;
      }
      break;
    }
  } /* switch (game->state) */
}

static void snake_render(game_t* game, SDL_Renderer* renderer) {
  assert(game);
  assert(game->data);

  snake_data_t* data = (snake_data_t*)game->data;
  vec2_t size;
  vec2_t pos;

  /* BOARD */
  pos = vec2_zero();
  size = vec2_scale(vec2(BOARD_WIDTH, BOARD_HEIGHT), BOARD_SCALE);
  render_rect(renderer, pos, size, color_board1);

  size = vec2_splat(BOARD_SCALE);
  for (int x = 0; x < BOARD_WIDTH; x++) {
    for (int y = 0; y < BOARD_HEIGHT; y++) {
      if ((x + y) % 2 == 0) {
        pos = vec2_scale(vec2(x, y), BOARD_SCALE);
        /* TODO: split set color and render rect */
        render_rect(renderer, pos, size, color_board2);
      }
    }
  }

  /* FOOD */
  pos = vec2_scale(data->food_pos, BOARD_SCALE);
  render_rect(renderer, pos, size, color_food);

  /* BODY */
  for (int i = 1; i < data->snake.length; i++) {
    pos = vec2_scale(data->snake.body[i], BOARD_SCALE);
    render_rect(renderer, pos, size, color_body);
  }

  /* HEAD */
  pos = vec2_scale(data->snake.body[0], BOARD_SCALE);
  render_rect(renderer, pos, size, color_head);

  if (game->state != GAME_RUNNING) {
    pos = vec2_scale(vec2(BOARD_WIDTH, BOARD_HEIGHT), BOARD_SCALE / 6.f);
    size = vec2_scale(pos, 4);
    render_rect(renderer, pos, size, color_menu);

    if (game->state == GAME_PAUSED) {
      render_text(
        renderer,
        vec2(
          (pos.x * 3) / 4,
          pos.y / 4 + 10
        ),
        vec2_splat(4.f),
        color_white,
        "PAUSED",
        ALIGN_MIDDLE
      );
      render_text(
        renderer,
        vec2(
          (pos.x * 3) / 2,
          pos.y
        ),
        vec2_splat(2.f),
        color_white,
        "PRESS <PAUSE> TO CONTINUE",
        ALIGN_MIDDLE
      );
    }
    else if (game->state == GAME_OVER) {
      render_text(
        renderer,
        vec2(
          (pos.x * 3) / 4,
          pos.y / 4 + 10
        ),
        vec2_splat(4.f),
        color_white,
        "GAME OVER",
        ALIGN_MIDDLE
      );
      render_text(
        renderer,
        vec2(
          (pos.x * 3) / 2,
          pos.y
        ),
        vec2_splat(2.f),
        color_white,
        "PRESS <ACTION> TO PLAY AGAIN",
        ALIGN_MIDDLE
      );
    }
    else if (game->state == GAME_WON) {
      render_text(
        renderer,
        vec2(
          (pos.x * 3) / 4,
          pos.y / 4 + 10
        ),
        vec2_splat(4.f),
        color_white,
        "YOU WON",
        ALIGN_MIDDLE
      );
      render_text(
        renderer,
        vec2(
          (pos.x * 3) / 2,
          pos.y
        ),
        vec2_splat(2.f),
        color_white,
        "PRESS <ACTION> TO PLAY AGAIN",
        ALIGN_MIDDLE
      );
    }
  }
}

static void snake_destroy(game_t* game) {
  assert(game);
  assert(game->data);

  free(game->data);
  game->data = NULL;
}

static void snake_data_init(snake_data_t* data, vec2_t win_size) {
  int start_x = BOARD_WIDTH / 4;
  int start_y = BOARD_HEIGHT / 2;

  *data = (snake_data_t){
    .snake = {
      .body = {
        { start_x, start_y },
        { start_x, start_y },
        { start_x, start_y }
      },
      .length = 3,
      .dir = RIGHT,
      .next = RIGHT
    },
    .food_pos = {
      start_x + 5, start_y
    },
    .accumulator = 0.f,
    .win_size = win_size
  };
}

static void snake_new_food(snake_data_t* data) {
  vec2_t pos;
  do {
    pos = vec2_rand(BOARD_WIDTH, BOARD_HEIGHT);
  } while (snake_contains(data, pos));
  data->food_pos = pos;
}

static void snake_update_dir(snake_data_t* data, input_t* input) {
  if (input->down[KEY_UP] && data->snake.dir != DOWN) {
    data->snake.next = UP;
  } else if (input->down[KEY_DOWN] && data->snake.dir != UP) {
    data->snake.next = DOWN;
  } else if (input->down[KEY_LEFT] && data->snake.dir != RIGHT) {
    data->snake.next = LEFT;
  } else if (input->down[KEY_RIGHT] && data->snake.dir != LEFT) {
    data->snake.next = RIGHT;
  }
}

static void snake_update_body(game_t* game) {
  snake_data_t* data = (snake_data_t*)game->data;

  for (int i = data->snake.length - 1; i > 0; i--) {
    data->snake.body[i] = data->snake.body[i - 1];
  }

  int dir_x = data->snake.dir == LEFT ? -1 :
              data->snake.dir == RIGHT ? 1 : 0;
  int dir_y = data->snake.dir == UP ? -1 :
              data->snake.dir == DOWN ? 1 : 0;

  vec2_t* head = &data->snake.body[0];

  head->x = head->x + dir_x;
  head->y = head->y + dir_y;

  /* out of bounds */
  if (head->x < 0 || head->x >= BOARD_WIDTH) {
    game->state = GAME_OVER;
    printf("game over x %f\n", head->x);
  }

  if (head->y < 0 || head->y >= BOARD_HEIGHT) {
    game->state = GAME_OVER;
    printf("game over y %f\n", head->y);
  }

  /* inside itself */
  /* it is not possible to collide with the first 3 body parts */
  /* and it should be impossible to collide with odd body parts */
  for (int i = 4; i < data->snake.length; i += 2) {
    if (vec2_equal(data->snake.body[i], *head)) {
      game->state = GAME_OVER;
      printf("game over snake %i\n", i);
    }
  }
}

static int snake_contains(snake_data_t* data, vec2_t pos) {
  for (int i = 0; i < data->snake.length; i++) {
    if (vec2_equal(data->snake.body[i], pos)) {
      return 1;
    }
  }
  return 0;
}

