#pragma once

#include <pebble.h>

bool disclaimer_window_needs_display(void);
void disclaimer_window_init(void (*on_accepted)(void));
void disclaimer_window_deinit(void);
