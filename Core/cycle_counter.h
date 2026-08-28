#ifndef CYCLE_COUNTER_H
#define CYCLE_COUNTER_H

#include <stdint.h>

void cycle_counter_init(void);
uint32_t cycle_counter_start(void);
uint32_t cycle_counter_stop(uint32_t start);

#endif
