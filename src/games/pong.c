#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "pong.h"

/*TODO MAX_SPEED or ray collision detection*/
#define MIN_FLOAT 0.01f
#define SPEED_MUL 1.01f
#define BASE_SPEED 200.f
#define BUF_SIZE 16

static void pong_init(game_t* game, vec2_t win_size);
static void pong_reset(game_t* game, vec2_t win_size);
static void pong_update(game_t* game, input_t* input, float dt);
static void pong_render(game_t* game, SDL_Renderer* renderer);
static void pong_destroy(game_t* game);
const struct game pong_template = {
  .init = pong_init,
  .reset = pong_reset,
  .update = pong_update,
  .render = pong_render,
  .destroy = pong_destroy,
  .data = NULL,
  .state = GAME_PAUSED,
  .color_bg = { 0, 0, 0, 255 }
};

struct pong_data {
  struct {
    vec2_t pos;
    vec2_t size;
    int score;
  } p1, p2;
  struct {
    vec2_t pos;
    vec2_t size;
    vec2_t vel;
  } ball;
  vec2_t win_size;
};

typedef struct pong_data pong_data_t;

static color_t color_player = { 255, 255, 255, 255 };
static color_t color_ball = { 205, 205, 255, 255 };
static color_t color_line = { 255, 255, 255, 255 };
static color_t color_menu = { 30, 30, 40, 255 };
static color_t color_white = { 255, 255, 255, 255 };

static void pong_data_init(pong_data_t* data, vec2_t win_size);
static void pong_clamp_player(pong_data_t* data, int p);
static void pong_soft_reset(game_t* data);
static int pong_equalf(float a, float b);

static void pong_init(game_t* game, vec2_t win_size) {
  assert(game);
  assert(!game->data);

  pong_data_t* data = calloc(1, sizeof(pong_data_t));
  game->data = data;

  pong_data_init(data, win_size);
}

static void pong_reset(game_t* game, vec2_t win_size) {
  assert(game);
  assert(game->data);

  memset(game->data, 0, sizeof(pong_data_t));
  pong_data_init((pong_data_t*)game->data, win_size);
}

static void pong_update(game_t* game, input_t* input, float dt) {
  assert(game);
  assert(game->data);

  pong_data_t* data = (pong_data_t*)game->data;

  if (game->state == GAME_RUNNING) {
    const float dis = BASE_SPEED * dt;

    /* BALL */
    data->ball.pos = vec2_add(data->ball.pos, vec2_scale(data->ball.vel, dis));
    const float bally = data->ball.pos.y;
    const float ballx = data->ball.pos.x;
    
    /* edge detection */
    if (bally <= 0 || bally + data->ball.size.y > data->win_size.y) {
      data->ball.vel.y = -data->ball.vel.y;
    }

    if (ballx <= 0) {
      data->p2.score++;
      pong_soft_reset(game);
    } else if (ballx + data->ball.size.x > data->win_size.x) {
      data->p1.score++;
      pong_soft_reset(game);
    }

    /* player collision */
    rect_t rect_ball = (rect_t){ data->ball.pos, data->ball.size };
    if (collision_aabb(rect_ball, rect(data->p1.pos, data->p1.size))) {
      data->ball.vel.x = -data->ball.vel.x;
      data->ball.pos.x = data->p1.pos.x + data->p1.size.x + MIN_FLOAT;
      data->ball.vel = vec2_scale(data->ball.vel, SPEED_MUL);
    } else if (collision_aabb(rect_ball, rect(data->p2.pos, data->p2.size))) {
      data->ball.vel.x = -data->ball.vel.x;
      data->ball.pos.x = data->p2.pos.x - data->ball.size.x - MIN_FLOAT;
      data->ball.vel = vec2_scale(data->ball.vel, SPEED_MUL);
    }

    /* PLAYER */
    if (input->down[KEY_UP]) {
      data->p1.pos.y -= dis;
    } else if (input->down[KEY_DOWN]) {
      data->p1.pos.y += dis;
    }

    pong_clamp_player(data, 1);

    /* BOT */
    const float boty = data->p2.pos.y + data->p2.size.y / 2;
    if (pong_equalf(bally, boty)) {
      /* Remove jittering */
    } else if (bally > boty) {
      data->p2.pos.y += dis;
    } else if (bally < boty) {
      data->p2.pos.y -= dis;
    }

    pong_clamp_player(data, 2);
  }
  else if (game->state == GAME_PAUSED) {
    if (input->pressed[KEY_ACTION]) {
      game->state = GAME_RUNNING;
    }
  }
}

static void pong_render(game_t* game, SDL_Renderer* renderer) {
  assert(game);
  assert(game->data);

  pong_data_t* data = (pong_data_t*)game->data;

  /* PLAYERS */
  render_rect(renderer, data->p1.pos, data->p1.size, color_player);
  render_rect(renderer, data->p2.pos, data->p2.size, color_player);

  /* BALL */
  render_rect(renderer, data->ball.pos, data->ball.size, color_ball);

  /* Line */
  int spacing = 20;
  int count = data->win_size.y / spacing;
  vec2_t size = vec2(2, 10);
  for (int i = 0; i < count; i++) {
    render_rect(
      renderer,
      vec2((data->win_size.x - size.x) / 2, spacing * i),
      size,
      color_line
    );
  }

  /* SCORES */
  char score[BUF_SIZE] = {
    [BUF_SIZE - 1] = '\0'
  };
  snprintf(score, BUF_SIZE, "Score: %i", data->p1.score);
  render_text(
    renderer,
    vec2(10, 10),
    vec2_splat(2.f),
    color_white,
    score,
    ALIGN_LEFT
  );
  snprintf(score, BUF_SIZE, "Score: %i", data->p2.score);
  render_text(
    renderer,
    vec2(data->win_size.x/2 - 10, 10),
    vec2_splat(2.f),
    color_white,
    score,
    ALIGN_RIGHT
  );

  if (game->state == GAME_PAUSED) {
    vec2_t pos = vec2_scale(data->win_size, 1 / 6.f);
    size = vec2_scale(pos, 4);
    render_rect(renderer, pos, size, color_menu);
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
      "PRESS <ACTION> TO CONTINUE",
      ALIGN_MIDDLE
    );
  }
}

static void pong_destroy(game_t* game) {
  assert(game);
  assert(game->data);

  free(game->data);
  game->data = NULL;
}

static void pong_data_init(pong_data_t* data, vec2_t win_size) {
  vec2_t size = vec2(20, win_size.y / 6);
  *data = (pong_data_t){
    .p1 = {
      .pos = vec2(10, win_size.y / 2),
      .size = size,
      .score = 0
    },
    .p2 = {
      .pos = vec2(win_size.x - 10 - size.x, win_size.y / 2),
      .size = size,
      .score = 0
    },
    .ball = {
      .pos = vec2(win_size.x / 2, win_size.y / 2),
      .size = vec2(20, 20),
      .vel = vec2(-1, 1)
    },
    .win_size = win_size
  };
}

static void pong_clamp_player(pong_data_t* data, int p) {
  if (p == 1) {
    const float p1y = data->p1.pos.y;
    data->p1.pos.y =
      p1y < 0 ? 0 :
        p1y > data->win_size.y - data->p1.size.y ? 
          data->win_size.y - data->p1.size.y : p1y;
  } else if (p == 2) {
    const float p2y = data->p2.pos.y;
    data->p2.pos.y =
      p2y < 0 ? 0 :
        p2y > data->win_size.y - data->p2.size.y ? 
          data->win_size.y - data->p2.size.y : p2y;
  }
}

static void pong_soft_reset(game_t* game) {
  pong_data_t* data = (pong_data_t*)game->data;

  data->ball.pos = vec2(data->win_size.x / 2, data->win_size.y / 2);
  data->ball.vel = vec2(-1, 1);
  data->p1.pos = vec2(10, data->win_size.y / 2);
  data->p2.pos = vec2(data->win_size.x - 10 - data->p2.size.x, data->win_size.y / 2);
  game->state = GAME_PAUSED;
}

#define EPSILON 1.f
static int pong_equalf(float a, float b) {
  return a + EPSILON > b && a - EPSILON < b;
}
