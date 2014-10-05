#include "callback.h"
#include "monkey.h"
#include "registers.h"

#include <stdbool.h>
#include <stddef.h>

function_t * current_program = initial_program;

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
        __interrupt_wait_go();
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


void restart_program(const char * message)
{
    puts(message);
    asm volatile("mov sp,%0\nbx %1" :: "r"(0x10089fe0), "r"(current_program));
    __builtin_unreachable();
}


void start_program(function_t * f)
{
    current_program = f;
    restart_program("");
}
