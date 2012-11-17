#ifndef CALLBACK_H_
#define CALLBACK_H_

typedef struct callback_record_t callback_record_t;
typedef void callback_function_t (callback_record_t * record);

struct callback_record_t {
    callback_record_t * next;
    callback_function_t * function;     // Null means not scheduled.
    void * data[];
};

// Only a single simple callback can be scheduled at once.
void callback_simple (void (*function)(void));

void callback_schedule (callback_function_t * function,
                        callback_record_t * record);

void callback_run (void);

#endif
