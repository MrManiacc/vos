#include "vtimer.h"
#include "containers/dict.h"
#include <time.h>
#include "vmem.h"
#include "vlogger.h"

typedef struct {
    time_t expiration_time;
    TimerCallback callback;
    void *data;
} TimerData;

static Dict *timers = NULL;

void initialize_timer() {
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
    
    DictIterator it = dict_iterator(timers);
    
    while (dict_next(&it)) {
        time_t current_time;
        time(&current_time);
        TimerData *data = (TimerData *) it.entry->value;
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
