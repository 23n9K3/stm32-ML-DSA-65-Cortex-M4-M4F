#include "mldsa_port.h"
#include "randombytes.h"

int mldsa65_keypair(uint8_t *pk, uint8_t *sk)
{
    int rc;
    randombytes_reset_status();
#if defined(MLDSA_IMPLEMENTATION_CLEAN)
    rc = PQCLEAN_MLDSA65_CLEAN_crypto_sign_keypair(pk, sk);
#else
    rc = crypto_sign_keypair(pk, sk);
#endif
    return (rc == 0 && randombytes_last_status() == 0) ? 0 : -1;
}

int mldsa65_sign(uint8_t *sig, size_t *sig_len,
                 const uint8_t *message, size_t message_len,
                 const uint8_t *sk)
{
    int rc;
    randombytes_reset_status();
#if defined(MLDSA_IMPLEMENTATION_CLEAN)
    rc = PQCLEAN_MLDSA65_CLEAN_crypto_sign_signature(
        sig, sig_len, message, message_len, sk);
#else
    rc = crypto_sign_signature(sig, sig_len, message, message_len, sk);
#endif
    if (rc != 0 || randombytes_last_status() != 0 || *sig_len != CRYPTO_BYTES) {
        return -1;
    }
    return 0;
}

int mldsa65_verify(const uint8_t *sig, size_t sig_len,
                   const uint8_t *message, size_t message_len,
                   const uint8_t *pk)
{
#if defined(MLDSA_IMPLEMENTATION_CLEAN)
    return PQCLEAN_MLDSA65_CLEAN_crypto_sign_verify(
        sig, sig_len, message, message_len, pk);
#else
    return crypto_sign_verify(sig, sig_len, message, message_len, pk);
#endif
}
