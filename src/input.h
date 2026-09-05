#pragma once

#include <stdbool.h>

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
