#ifndef MLDSA_PORT_H
#define MLDSA_PORT_H

#include <stddef.h>
#include <stdint.h>

#if defined(MLDSA_IMPLEMENTATION_CLEAN) && defined(MLDSA_IMPLEMENTATION_M4F)
#error "Select only one ML-DSA implementation"
#endif

#if !defined(MLDSA_IMPLEMENTATION_CLEAN) && !defined(MLDSA_IMPLEMENTATION_M4F)
#define MLDSA_IMPLEMENTATION_CLEAN 1
#endif

#if defined(MLDSA_IMPLEMENTATION_CLEAN)
#include "api.h"
#define CRYPTO_PUBLICKEYBYTES PQCLEAN_MLDSA65_CLEAN_CRYPTO_PUBLICKEYBYTES
#define CRYPTO_SECRETKEYBYTES PQCLEAN_MLDSA65_CLEAN_CRYPTO_SECRETKEYBYTES
#define CRYPTO_BYTES PQCLEAN_MLDSA65_CLEAN_CRYPTO_BYTES
#define MLDSA65_ALGNAME PQCLEAN_MLDSA65_CLEAN_CRYPTO_ALGNAME
#else
/* The original pqm4 m4f api.h uses these generic size macros. */
#include "m4f/api.h"
#define MLDSA65_ALGNAME "ML-DSA-65 (pqm4 m4f)"
#endif

int mldsa65_keypair(uint8_t *pk, uint8_t *sk);
int mldsa65_sign(uint8_t *sig, size_t *sig_len,
                 const uint8_t *message, size_t message_len,
                 const uint8_t *sk);
int mldsa65_verify(const uint8_t *sig, size_t sig_len,
                   const uint8_t *message, size_t message_len,
                   const uint8_t *pk);

#endif
