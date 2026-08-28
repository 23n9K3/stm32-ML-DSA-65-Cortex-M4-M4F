#include "mldsa_test.h"
#include "cycle_counter.h"
#include "mldsa_port.h"
#include "randombytes.h"
#include "stm32l4xx_hal.h"
#include "uart_log.h"
#include <string.h>

static uint8_t public_key[CRYPTO_PUBLICKEYBYTES];
static uint8_t secret_key[CRYPTO_SECRETKEYBYTES];
static uint8_t signature[CRYPTO_BYTES];
static size_t signature_len;

static const uint8_t test_message[] =
    "STM32L4R5 ML-DSA-65 verification test";
static uint8_t modified_message[sizeof(test_message)];

static void log_time(const char *name, uint32_t cycles)
{
    uint32_t hz = SystemCoreClock;
    uint32_t ms_whole = cycles / (hz / 1000U);
    uint32_t ms_frac = ((cycles % (hz / 1000U)) * 1000U) / (hz / 1000U);
    uart_log_printf("[TIME] %s cycles = %lu (%lu.%03lu ms)\r\n",
                    name, (unsigned long)cycles,
                    (unsigned long)ms_whole, (unsigned long)ms_frac);
}

static int run_repeat_iteration(unsigned int iteration)
{
    if (mldsa65_keypair(public_key, secret_key) != 0) {
        uart_log_printf("[ERROR] repeat %u: crypto_sign_keypair failed\r\n", iteration);
        return -1;
    }
    if (mldsa65_sign(signature, &signature_len,
                     test_message, sizeof(test_message) - 1U,
                     secret_key) != 0) {
        uart_log_printf("[ERROR] repeat %u: crypto_sign_signature failed\r\n", iteration);
        return -1;
    }
    if (mldsa65_verify(signature, signature_len,
                       test_message, sizeof(test_message) - 1U,
                       public_key) != 0) {
        uart_log_printf("[ERROR] repeat %u: valid signature was rejected\r\n", iteration);
        return -1;
    }
    uart_log_printf("[PASS] Repeat %u/%u keypair/sign/verify\r\n",
                    iteration, (unsigned int)MLDSA_TEST_REPEAT_COUNT);
    return 0;
}

int mldsa_run_self_test(void)
{
    int failures = 0;
    int rc;
    uint32_t start;
    uint32_t cycles;
    unsigned int i;

    memset(public_key, 0, sizeof(public_key));
    memset(secret_key, 0, sizeof(secret_key));
    memset(signature, 0, sizeof(signature));
    memset(modified_message, 0, sizeof(modified_message));
    signature_len = 0U;

    uart_log_write("[BOOT] ML-DSA-65 test start\r\n");
#if MLDSA_TARGET_M4F
    uart_log_write("[BUILD] CPU: CORTEX-M4F\r\n");
#else
    uart_log_write("[BUILD] CPU: CORTEX-M4\r\n");
#endif
#if MLDSA_USE_NTT_ASM
    uart_log_write("[BUILD] ML-DSA implementation: ASM M4F\r\n");
    uart_log_write("[BUILD] Keccak implementation: M4 ASM\r\n");
    uart_log_write("[BUILD] NTT implementation: M4F ASM\r\n");
#elif MLDSA_USE_KECCAK_ASM
    uart_log_write("[BUILD] ML-DSA implementation: ASM M4\r\n");
    uart_log_write("[BUILD] Keccak implementation: M4 ASM\r\n");
    uart_log_write("[BUILD] NTT implementation: C\r\n");
#else
#if MLDSA_TARGET_M4F
    uart_log_write("[BUILD] ML-DSA implementation: CLEAN C M4F\r\n");
#else
    uart_log_write("[BUILD] ML-DSA implementation: CLEAN C M4\r\n");
#endif
    uart_log_write("[BUILD] Keccak implementation: C\r\n");
    uart_log_write("[BUILD] NTT implementation: C\r\n");
#endif
    uart_log_printf("[INFO] Algorithm: %s\r\n", MLDSA65_ALGNAME);
    uart_log_printf("[INFO] Public key size: %lu\r\n", (unsigned long)CRYPTO_PUBLICKEYBYTES);
    uart_log_printf("[INFO] Secret key size: %lu\r\n", (unsigned long)CRYPTO_SECRETKEYBYTES);
    uart_log_printf("[INFO] Signature size: %lu\r\n", (unsigned long)CRYPTO_BYTES);
#if MLDSA_USE_DETERMINISTIC_TEST_RNG
    uart_log_write("[WARNING] Deterministic test RNG enabled\r\n");
    uart_log_write("[WARNING] This mode is not secure\r\n");
#else
    uart_log_write("[INFO] Random source: STM32 hardware RNG\r\n");
#endif

    uart_log_write("[TEST] Keypair generation start\r\n");
    start = cycle_counter_start();
    rc = mldsa65_keypair(public_key, secret_key);
    cycles = cycle_counter_stop(start);
    log_time("keypair", cycles);
    uart_log_printf("[INFO] keypair return = %d\r\n", rc);
    if (rc != 0) {
        uart_log_write("[ERROR] crypto_sign_keypair failed\r\n");
        return -1;
    }
    uart_log_write("[TEST] Keypair generation success\r\n");
    uart_log_hex("Public key", public_key, sizeof(public_key), 16U);

    uart_log_write("[TEST] Signing start\r\n");
    start = cycle_counter_start();
    rc = mldsa65_sign(signature, &signature_len,
                      test_message, sizeof(test_message) - 1U,
                      secret_key);
    cycles = cycle_counter_stop(start);
    log_time("sign", cycles);
    uart_log_printf("[INFO] sign return = %d, signature length = %lu\r\n",
                    rc, (unsigned long)signature_len);
    if (rc != 0) {
        uart_log_write("[ERROR] crypto_sign_signature failed\r\n");
        return -1;
    }
    uart_log_write("[TEST] Signing success\r\n");
    uart_log_hex("Signature", signature, signature_len, 16U);

    uart_log_write("[TEST] Verification start\r\n");
    start = cycle_counter_start();
    rc = mldsa65_verify(signature, signature_len,
                        test_message, sizeof(test_message) - 1U,
                        public_key);
    cycles = cycle_counter_stop(start);
    log_time("verify", cycles);
    if (rc == 0) {
        uart_log_write("[PASS] Valid signature accepted\r\n");
    } else {
        uart_log_write("[ERROR] valid signature was rejected\r\n");
        ++failures;
    }

    memcpy(modified_message, test_message, sizeof(test_message));
    modified_message[0] ^= 0x01U;
    rc = mldsa65_verify(signature, signature_len,
                        modified_message, sizeof(test_message) - 1U,
                        public_key);
    if (rc != 0) {
        uart_log_write("[PASS] Modified message rejected\r\n");
    } else {
        uart_log_write("[ERROR] modified message was accepted\r\n");
        ++failures;
    }

    signature[0] ^= 0x01U;
    rc = mldsa65_verify(signature, signature_len,
                        test_message, sizeof(test_message) - 1U,
                        public_key);
    if (rc != 0) {
        uart_log_write("[PASS] Modified signature rejected\r\n");
    } else {
        uart_log_write("[ERROR] modified signature was accepted\r\n");
        ++failures;
    }

    signature[0] ^= 0x01U;
    rc = mldsa65_verify(signature, signature_len,
                        test_message, sizeof(test_message) - 1U,
                        public_key);
    if (rc == 0) {
        uart_log_write("[PASS] Restored signature accepted\r\n");
    } else {
        uart_log_write("[ERROR] restored signature was rejected\r\n");
        ++failures;
    }

    if (failures == 0) {
        uart_log_printf("[TEST] Stability test: %u repetitions\r\n",
                        (unsigned int)MLDSA_TEST_REPEAT_COUNT);
        for (i = 1U; i <= MLDSA_TEST_REPEAT_COUNT; ++i) {
            if (run_repeat_iteration(i) != 0) {
                ++failures;
                break;
            }
        }
    }

    memset(secret_key, 0, sizeof(secret_key));
    return failures == 0 ? 0 : -1;
}
