#include "callback.h"
#include "registers.h"

#include <stdbool.h>
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
    __interrupt_disable();
    if (next_callback == NULL)
        callback_tail = &next_callback;

    if (!callback->function) {
        *callback_tail = callback;
        callback_tail = &callback->next;
    }
    callback->function = function;
    __interrupt_enable();
}


void callback_wait (void)
{
    if (next_callback == NULL) {
        __interrupt_wait();
        __interrupt_enable();
        __interrupt_disable();
        return;
    }
    callback_record_t * callback = next_callback;
    next_callback = callback->next;
    callback_function_t * function = callback->function;
    callback->function = NULL;
    __interrupt_enable();
    function (callback);
    __interrupt_disable();
}
