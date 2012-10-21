#ifndef FREQ_H_
#define FREQ_H_

// Frequency measurement of a given clock.
// @c multiplier is the return value for a 1MHz clock.
int frequency (unsigned clock, unsigned multiplier);

void clock_report (void);

#endif
