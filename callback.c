#include "callback.h"

#include <stddef.h>

static callback_record_t * next_callback;
static callback_record_t ** callback_tail;

void callback_simple (void (*function) (void))
{
    static struct callback_record_t record;
    callback_schedule ((callback_function_t *) function, &record);
}


void callback_schedule (callback_function_t * function,
                        callback_record_t * callback)
{
    asm volatile ("cpsid i\n" ::: "memory");
    if (next_callback == NULL)
        callback_tail = &next_callback;

    if (!callback->function) {
        *callback_tail = callback;
        callback_tail = &callback->next;
    }
    callback->function = function;
    asm volatile ("cpsie i\n" ::: "memory");
}

void callback_run (void)
{
    asm volatile ("cpsid i\n" ::: "memory");
    while (next_callback) {
        callback_record_t * callback = next_callback;
        next_callback = callback->next;
        callback_function_t * function = callback->function;
        callback->function = NULL;
        asm volatile ("cpsie i\n" ::: "memory");
        function (callback);
        asm volatile ("cpsid i\n" ::: "memory");
    }
    asm volatile ("cpsie i\n" ::: "memory");
}
