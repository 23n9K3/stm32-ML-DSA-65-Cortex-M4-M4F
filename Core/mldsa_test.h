#ifndef MLDSA_TEST_H
#define MLDSA_TEST_H

#ifndef MLDSA_TEST_REPEAT_COUNT
#define MLDSA_TEST_REPEAT_COUNT 10U
#endif

#ifndef MLDSA_USE_KECCAK_ASM
#define MLDSA_USE_KECCAK_ASM 0
#endif

#ifndef MLDSA_USE_NTT_ASM
#define MLDSA_USE_NTT_ASM 0
#endif

#ifndef MLDSA_TARGET_M4F
#define MLDSA_TARGET_M4F 0
#endif

int mldsa_run_self_test(void);

#endif
