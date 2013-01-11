#ifndef CALLBACK_H_
#define CALLBACK_H_

typedef struct callback_record_t callback_record_t;
typedef void callback_function_t (callback_record_t * record);

struct callback_record_t {
    callback_record_t * next;
    callback_function_t * function;     // Null means not scheduled.
    unsigned udata;                     // User data.
    void * data[];
};

// Only a single simple callback can be scheduled at once.
void callback_simple (void (*function)(void));

void callback_schedule (callback_function_t * function,
                        callback_record_t * record);

// Either run a callback or wait for an interrupt.  Interrupts should be
// disabled on entry.
void callback_wait (void);

#endif