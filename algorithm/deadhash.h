
#ifndef DEADHASH_H
#define DEADHASH_H

#include <stdint.h>
#include <stddef.h>


/*
 * ============================================
 * DEADHASH VERSION
 * ============================================
 */

#define DEADHASH_VERSION 1


/*
 * ============================================
 * OUTPUT / SALT
 * ============================================
 */

#define DEADHASH_OUTPUT_SIZE 32
#define DEADHASH_SALT_SIZE 16


/*
 * ============================================
 * DEFAULT PARAMETERS
 * ============================================
 */

#define DEADHASH_ROUNDS 100000

#define DEADHASH_MEMORY_BLOCKS 1024

#define DEADHASH_BLOCK_SIZE 32


/*
 * ============================================
 * PARAMETER LIMITS
 * ============================================
 */

#define DEADHASH_MIN_ROUNDS 1000
#define DEADHASH_MAX_ROUNDS 1000000

#define DEADHASH_MIN_MEMORY_BLOCKS 64
#define DEADHASH_MAX_MEMORY_BLOCKS 32768


/*
 * ============================================
 * STORED RECORD
 * ============================================
 */

#define DEADHASH_MAX_STRING_SIZE 256


/*
 * ============================================
 * RANDOM SALT
 * ============================================
 */

int deadhash_generate_salt(
    uint8_t *salt,
    size_t length
);


/*
 * ============================================
 * HASHING
 * ============================================
 */

int deadhash_with_params(
    const char *password,
    const uint8_t *salt,
    uint32_t rounds,
    uint32_t memory_blocks,
    uint8_t *output
);


/*
 * Default DEADHASH configuration.
 */
void deadhash(
    const char *password,
    const uint8_t *salt,
    uint8_t *output
);


/*
 * ============================================
 * VERIFICATION
 * ============================================
 */

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
 * ============================================
 * HEX ENCODING
 * ============================================
 */

void deadhash_hex_encode(
    const uint8_t *input,
    size_t input_length,
    char *output
);

#endif


