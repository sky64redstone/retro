#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct save save_t;
struct save {
  char* directory;
};

bool save_init(save_t* save, const char* app);
void save_destroy(save_t* save);

bool save_set(
  const save_t* save, const char* key, const void* data, size_t size
);
void* save_get(const save_t* save, const char* key, size_t* size);
bool save_has(const save_t* save, const char* key);
bool save_remove(const save_t* save, const char* key);
