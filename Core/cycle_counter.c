#include "cycle_counter.h"
#include "stm32l4xx.h"

void cycle_counter_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

uint32_t cycle_counter_start(void)
{
    return DWT->CYCCNT;
}

uint32_t cycle_counter_stop(uint32_t start)
{
    return DWT->CYCCNT - start;
}
