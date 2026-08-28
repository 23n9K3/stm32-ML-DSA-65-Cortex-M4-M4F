#include "keccak_kat.h"

#include "fips202.h"
#include "uart_log.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef MLDSA_USE_KECCAK_ASM
#define MLDSA_USE_KECCAK_ASM 0
#endif

#if MLDSA_USE_KECCAK_ASM
#define KAT_OUTPUT_BYTES 32U

typedef void (*shake_fn)(uint8_t *, size_t, const uint8_t *, size_t);

static const uint8_t shake128_empty[KAT_OUTPUT_BYTES] = {
    0x7f, 0x9c, 0x2b, 0xa4, 0xe8, 0x8f, 0x82, 0x7d,
    0x61, 0x60, 0x45, 0x50, 0x76, 0x05, 0x85, 0x3e,
    0xd7, 0x3b, 0x80, 0x93, 0xf6, 0xef, 0xbc, 0x88,
    0xeb, 0x1a, 0x6e, 0xac, 0xfa, 0x66, 0xef, 0x26
};
static const uint8_t shake256_empty[KAT_OUTPUT_BYTES] = {
    0x46, 0xb9, 0xdd, 0x2b, 0x0b, 0xa8, 0x8d, 0x13,
    0x23, 0x3b, 0x3f, 0xeb, 0x74, 0x3e, 0xeb, 0x24,
    0x3f, 0xcd, 0x52, 0xea, 0x62, 0xb8, 0x1b, 0x82,
    0xb5, 0x0c, 0x27, 0x64, 0x6e, 0xd5, 0x76, 0x2f
};
static const uint8_t shake128_abc[KAT_OUTPUT_BYTES] = {
    0x58, 0x81, 0x09, 0x2d, 0xd8, 0x18, 0xbf, 0x5c,
    0xf8, 0xa3, 0xdd, 0xb7, 0x93, 0xfb, 0xcb, 0xa7,
    0x40, 0x97, 0xd5, 0xc5, 0x26, 0xa6, 0xd3, 0x5f,
    0x97, 0xb8, 0x33, 0x51, 0x94, 0x0f, 0x2c, 0xc8
};
static const uint8_t shake256_abc[KAT_OUTPUT_BYTES] = {
    0x48, 0x33, 0x66, 0x60, 0x13, 0x60, 0xa8, 0x77,
    0x1c, 0x68, 0x63, 0x08, 0x0c, 0xc4, 0x11, 0x4d,
    0x8d, 0xb4, 0x45, 0x30, 0xf8, 0xf1, 0xe1, 0xee,
    0x4f, 0x94, 0xea, 0x37, 0xe7, 0x8b, 0x57, 0x39
};

static int run_one(const char *name, shake_fn fn, const uint8_t *input,
                   size_t input_len, const uint8_t expected[KAT_OUTPUT_BYTES])
{
    uint8_t output[KAT_OUTPUT_BYTES];
    fn(output, sizeof(output), input, input_len);
    if (memcmp(output, expected, sizeof(output)) != 0) {
        uart_log_printf("[KECCAK] %s: FAIL\r\n", name);
        uart_log_hex("SHAKE actual", output, sizeof(output), sizeof(output));
        return -1;
    }
    uart_log_printf("[KECCAK] %s: PASS\r\n", name);
    return 0;
}

int keccak_kat_run(void)
{
    static const uint8_t empty[1] = {0U};
    static const uint8_t abc[] = {'a', 'b', 'c'};
    int failures = 0;

    uart_log_write("[KECCAK] ASM test start\r\n");
    failures += run_one("SHAKE128 empty", shake128, empty, 0U,
                        shake128_empty) != 0;
    failures += run_one("SHAKE256 empty", shake256, empty, 0U,
                        shake256_empty) != 0;
    failures += run_one("SHAKE128 abc", shake128, abc, sizeof(abc),
                        shake128_abc) != 0;
    failures += run_one("SHAKE256 abc", shake256, abc, sizeof(abc),
                        shake256_abc) != 0;

    if (failures == 0) {
        uart_log_write("[KECCAK] ALL TESTS PASSED\r\n");
        return 0;
    }
    uart_log_write("[KECCAK] TEST FAILED\r\n");
    return -1;
}
#else
int keccak_kat_run(void)
{
    uart_log_write("[KECCAK] KAT skipped for CLEAN baseline\r\n");
    return 0;
}
#endif
