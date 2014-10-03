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

typedef void function_t(void);
extern function_t * current_program;

// Initialise stack and call current_program.
void _Noreturn restart_program(const char * p);

void _Noreturn start_program(function_t * f);

extern function_t _Noreturn initial_program;

#endif
