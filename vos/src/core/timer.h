/**
 * Created by jraynor on 8/27/2023.
 */
#pragma once
#include "defines.h"

typedef void (*TimerCallback)(const char *id);

void timer_initialize();
void timer_set(const char *id, u32 delay, TimerCallback callback);
void timer_poll();
void timer_cleanup();
