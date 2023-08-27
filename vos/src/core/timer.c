#include "timer.h"
#include "containers/dict.h"
#include <time.h>
#include <pthread.h>
#include "core/mem.h"
#include "logger.h"

typedef struct {
  time_t expiration_time;
  TimerCallback callback;
} TimerData;

static dict *timers = NULL;

void timer_initialize() {
    if (timers != NULL) {
        timer_cleanup();
        vwarn("Timer was already initialized, cleaning up old timer");
        return;
    }
    timers = dict_create_default();
}

void timer_set(const char *id, u32 delay, TimerCallback callback) {
    if (!timers) return;
    time_t current_time;
    time(&current_time);

    TimerData *data = kallocate(sizeof(TimerData), MEMORY_TAG_DICT);
    //converts the delay into miliseconds from second
    delay /= 1000;
    data->expiration_time = current_time + delay;
    data->callback = callback;
    if (dict_lookup(timers, id)) {
        return;
    }

    dict_insert(timers, id, data);

}

void timer_poll() {
    if (!timers) return;

    idict it = dict_iterator(timers);

    // List of keys to be removed after iteration
    char **keys_to_remove = NULL;
    u64 remove_count = 0;

    while (dict_next(&it)) {
        time_t current_time;
        time(&current_time);
        TimerData *data = (TimerData *) it.entry->object;
        if (current_time >= data->expiration_time) {
            data->callback(it.entry->key);
            dict_remove(timers, it.entry->key);
        }
    }

//    // Now, remove the keys from the dictionary
//    for (u64 i = 0; i < remove_count; i++) {
//        dict_remove(timers, keys_to_remove[i]);
//    }
//
//    // Free the list of keys to remove
//    kfree(keys_to_remove, sizeof(char *) * remove_count, MEMORY_TAG_DICT);
}

void timer_cleanup() {
    if (timers) {
        dict_destroy(timers);
        timers = NULL;
    }
}
