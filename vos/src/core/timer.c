#include "timer.h"
#include "containers/Map.h"
#include <time.h>
#include <pthread.h>
#include "core/mem.h"
#include "logger.h"

typedef struct {
  time_t expiration_time;
  TimerCallback callback;
  void *data;
} TimerData;

static Map *timers = NULL;

void timer_initialize() {
    if (timers != NULL) {
        timer_cleanup();
        vwarn("Timer was already initialized, cleaning up old timer");
        return;
    }
    timers = dict_create_default();
}

void timer_set(const char *id, u32 delay, TimerCallback callback, void *raw_data) {
    if (!timers) return;
    time_t current_time;
    time(&current_time);

    TimerData *data = kallocate(sizeof(TimerData), MEMORY_TAG_DICT);
    //converts the delay into miliseconds from second
    delay /= 1000;
    data->expiration_time = current_time + delay;
    data->callback = callback;
    data->data = raw_data;
    dict_set(timers, id, data);
}

b8 timer_exists(const char *id) {
    if (!timers) return false;
    return NULL != dict_get(timers, id);
}

void timer_poll() {
    if (!timers) return;

    idict it = dict_iterator(timers);

    while (dict_next(&it)) {
        time_t current_time;
        time(&current_time);
        TimerData *data = (TimerData *) it.entry->object;
        if (current_time >= data->expiration_time) {
            data->callback(data->data);
            dict_remove(timers, it.entry->key);
        }
    }
}

void timer_cleanup() {
    if (timers) {
        dict_destroy(timers);
        timers = NULL;
    }
}
