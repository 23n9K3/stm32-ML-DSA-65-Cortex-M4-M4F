#include "randombytes.h"
#include "main.h"
#include "uart_log.h"
#include <string.h>

extern RNG_HandleTypeDef hrng;

static volatile int rng_status;

void randombytes_reset_status(void)
{
    rng_status = 0;
}

int randombytes_last_status(void)
{
    return rng_status;
}

#if MLDSA_USE_DETERMINISTIC_TEST_RNG
static uint32_t deterministic_state = 0x6D4C4453U;

static uint32_t deterministic_word(void)
{
    uint32_t x = deterministic_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    deterministic_state = x;
    return x;
}
#endif

int randombytes(uint8_t *out, size_t outlen)
{
    while (outlen != 0U) {
        uint32_t word;
        size_t take = outlen < sizeof(word) ? outlen : sizeof(word);
#if MLDSA_USE_DETERMINISTIC_TEST_RNG
        word = deterministic_word();
#else
        if (HAL_RNG_GenerateRandomNumber(&hrng, &word) != HAL_OK) {
            rng_status = -1;
            memset(out, 0, outlen);
            uart_log_printf("[ERROR] HAL_RNG_GenerateRandomNumber failed, code=0x%08lX\r\n",
                            (unsigned long)HAL_RNG_GetError(&hrng));
            return -1;
        }
#endif
        memcpy(out, &word, take);
        out += take;
        outlen -= take;
    }
    return 0;
}
