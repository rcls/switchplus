#ifndef FREQ_H_
#define FREQ_H_

// Frequency measurement of a given clock.
// @c multiplier is the return value for a 1MHz clock.  So 1 gives frequency
// in MHz, 1000 gives frequency in kHz.
int frequency (unsigned clock, unsigned multiplier);
int cpu_frequency (unsigned multiplier);

void clock_report (void);

#endif
