#pragma once

#include <pebble.h>

#include "../layers/progress_layer.h"

#define PROGRESS_LAYER_WINDOW_DELTA 33
#define PROGRESS_LAYER_WINDOW_WIDTH 80

void splash_deinit(void);
void splash_init(void);
void splash_update_progress(int progress_percent);