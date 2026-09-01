
#ifndef DEADHASH_H
#define DEADHASH_H

#include <stdint.h>
#include <stddef.h>

/*
 * ============================================================
 * DEADHASH - EDUCATIONAL PASSWORD HASH + FILE ENCRYPTION
 * ============================================================
 *
 * WARNING:
 *
 * This implementation is educational.
 *
 * DO NOT use it for protecting real secrets.
 *
 * Real applications should use a reviewed construction such as:
 *
 *     Argon2id + XChaCha20-Poly1305
 *
 * ============================================================
 */


/*
 * ============================================================
 * HASH PARAMETERS
 * ============================================================
 */

#define DEADHASH_VERSION 1

#define DEADHASH_OUTPUT_SIZE 32
#define DEADHASH_SALT_SIZE 16

#define DEADHASH_ROUNDS 100000
#define DEADHASH_MEMORY_BLOCKS 1024
#define DEADHASH_BLOCK_SIZE 32

#define DEADHASH_MIN_ROUNDS 1000
#define DEADHASH_MAX_ROUNDS 1000000

#define DEADHASH_MIN_MEMORY_BLOCKS 64
#define DEADHASH_MAX_MEMORY_BLOCKS 32768

#define DEADHASH_MAX_STRING_SIZE 512


/*
 * ============================================================
 * FILE ENCRYPTION PARAMETERS
 * ============================================================
 */

/*
 * Salt used for deriving the encryption key.
 */
#define DEADHASH_FILE_SALT_SIZE 16


/*
 * Nonce used by the educational stream cipher.
 */
#define DEADHASH_NONCE_SIZE 16


/*
 * Derived encryption key.
 */
#define DEADHASH_KEY_SIZE 32


/*
 * Number of bytes processed at once.
 */
#define DEADHASH_FILE_BUFFER_SIZE 4096


/*
 * ============================================================
 * PASSWORD HASHING
 * ============================================================
 */

int deadhash_generate_salt(
    uint8_t *salt,
    size_t length
);


int deadhash_with_params(
    const char *password,
    const uint8_t *salt,
    uint32_t rounds,
    uint32_t memory_blocks,
    uint8_t *output
);


void deadhash(
    const char *password,
    const uint8_t *salt,
    uint8_t *output
);


int deadhash_verify(
    const char *password,
    const uint8_t *salt,
    const uint8_t *stored_hash
);


int deadhash_verify_record(
    const char *password,
    const char *record
);


/*
 * ============================================================
 * HEX ENCODING
 * ============================================================
 */

void deadhash_hex_encode(
    const uint8_t *input,
    size_t input_length,
    char *output
);


/*
 * ============================================================
 * EDUCATIONAL FILE ENCRYPTION
 * ============================================================
 */

int deadhash_encrypt_file(
    const char *input_path,
    const char *output_path,
    const char *password
);


/*
 * ============================================================
 * EDUCATIONAL FILE DECRYPTION
 * ============================================================
 */

int deadhash_decrypt_file(
    const char *input_path,
    const char *output_path,
    const char *password
);

#endif

