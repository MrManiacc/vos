/**
 * Created by jraynor on 8/27/2023.
 */
#pragma once
#include "defines.h"

typedef void (*TimerCallback)(void *data);

void initialize_timer();
void timer_set(const char *id, u32 delay, TimerCallback callback, void *raw_data);
b8 timer_exists(const char *id);
void timer_poll();
void timer_cleanup();
