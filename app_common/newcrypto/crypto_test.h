#ifndef _CRYPTO_TEST_H_
#define _CRYPTO_TEST_H_

#include <stdint.h>

bool testSignOpen();
bool testEncrypt();
bool testDecryptSingle(uint8_t * cmsg_plus_nonce, uint16_t datalen);
bool testDecryptMultiple(uint8_t * cmsg_plus_nonce, uint16_t inlen);

#endif /* end crypto_test.h */
