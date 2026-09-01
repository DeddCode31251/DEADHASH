
#include "deadhash.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/*
 * ============================================
 * ROTATE LEFT
 * ============================================
 */

static uint32_t rotate_left(
    uint32_t value,
    unsigned int amount
)
{
    return (value << amount) |
           (value >> (32 - amount));
}


/*
 * ============================================
 * STATE MIXING
 * ============================================
 */

static void mix_state(
    uint32_t state[8],
    uint32_t input
)
{
    state[0] ^= input;

    state[1] += state[0];

    state[1] =
        rotate_left(
            state[1],
            5
        );


    state[2] ^= state[1];

    state[2] *=
        0x9E3779B1;


    state[3] += state[2];

    state[3] =
        rotate_left(
            state[3],
            7
        );


    state[4] ^= state[3];

    state[4] *=
        0x85EBCA6B;


    state[5] += state[4];

    state[5] =
        rotate_left(
            state[5],
            11
        );


    state[6] ^= state[5];

    state[6] *=
        0xC2B2AE35;


    state[7] += state[6];

    state[7] =
        rotate_left(
            state[7],
            13
        );
}


/*
 * ============================================
 * SECURE RANDOM SALT
 * ============================================
 */

int deadhash_generate_salt(
    uint8_t *salt,
    size_t length
)
{
    FILE *random_source =
        fopen(
            "/dev/urandom",
            "rb"
        );


    if (random_source == NULL) {
        return 0;
    }


    size_t bytes_read =
        fread(
            salt,
            1,
            length,
            random_source
        );


    fclose(random_source);


    return bytes_read == length;
}


/*
 * ============================================
 * INITIALIZE MEMORY
 * ============================================
 */

static void fill_memory(
    uint32_t state[8],
    uint8_t *memory,
    uint32_t memory_blocks
)
{
    for (
        uint32_t block = 0;
        block < memory_blocks;
        block++
    )
    {
        for (
            size_t i = 0;
            i < DEADHASH_BLOCK_SIZE;
            i++
        )
        {
            /*
             * Use the current state.
             */
            uint32_t value =
                state[
                    (i + block) % 8
                ];


            /*
             * Mix block number.
             */
            value ^= block;


            /*
             * Mix byte position.
             */
            value ^=
                (uint32_t)i << 8;


            /*
             * Non-linear transformation.
             */
            value =
                rotate_left(
                    value,
                    (unsigned int)(
                        (i % 31) + 1
                    )
                );


            value *=
                0x9E3779B1;


            /*
             * Store byte.
             */
            memory[
                (size_t)block *
                DEADHASH_BLOCK_SIZE +
                i
            ] =
                (uint8_t)value;


            /*
             * Feed generated memory
             * back into the state.
             */
            state[i % 8] ^=
                value;
        }
    }
}


/*
 * ============================================
 * STATE-DEPENDENT MEMORY ACCESS
 * ============================================
 *
 * This is the important new part.
 *
 * Instead of:
 *
 *     block 0
 *     block 1
 *     block 2
 *     block 3
 *
 * We derive the next block from
 * the current internal state.
 */

static uint32_t select_memory_block(
    const uint32_t state[8],
    uint32_t memory_blocks,
    uint32_t round,
    uint32_t lane
)
{
    uint32_t value =
        state[
            (round + lane) % 8
        ];


    value ^=
        state[
            (round + lane + 3) % 8
        ];


    value +=
        round *
        0x9E3779B1;


    value ^=
        lane *
        0x85EBCA6B;


    value =
        rotate_left(
            value,
            (unsigned int)(
                (round + lane) % 31 + 1
            )
        );


    /*
     * Convert state into a valid
     * memory block index.
     */
    return value % memory_blocks;
}


/*
 * ============================================
 * MIX ONE MEMORY BLOCK
 * ============================================
 */

static void mix_memory_block(
    uint32_t state[8],
    uint8_t *memory,
    uint32_t block
)
{
    size_t offset =
        (size_t)block *
        DEADHASH_BLOCK_SIZE;


    for (
        size_t i = 0;
        i < DEADHASH_BLOCK_SIZE;
        i++
    )
    {
        /*
         * Read byte from selected block.
         */
        uint32_t value =
            memory[
                offset + i
            ];


        /*
         * Mix memory into state.
         */
        mix_state(
            state,
            value
        );


        /*
         * Generate a new byte
         * from the updated state.
         */
        uint32_t generated =
            state[
                i % 8
            ];


        generated ^=
            value;


        generated =
            rotate_left(
                generated,
                (unsigned int)(
                    (i % 13) + 1
                )
            );


        /*
         * Write it back.
         *
         * This makes the memory state
         * evolve during hashing.
         */
        memory[
            offset + i
        ] =
            (uint8_t)generated;
    }
}


/*
 * ============================================
 * MEMORY-HARD MIXING
 * ============================================
 */

static void memory_hard_mix(
    uint32_t state[8],
    uint8_t *memory,
    uint32_t memory_blocks,
    uint32_t rounds
)
{
    /*
     * Number of memory passes.
     *
     * We don't use the full CPU round
     * count here because that would make
     * this educational implementation
     * unnecessarily slow.
     */
    uint32_t memory_passes =
        rounds / 1000;


    if (memory_passes < 1) {
        memory_passes = 1;
    }


    /*
     * Cap memory passes.
     */
    if (memory_passes > 1024) {
        memory_passes = 1024;
    }


    for (
        uint32_t pass = 0;
        pass < memory_passes;
        pass++
    )
    {
        /*
         * Number of state-dependent
         * memory accesses per pass.
         */
        uint32_t accesses =
            memory_blocks;


        for (
            uint32_t access = 0;
            access < accesses;
            access++
        )
        {
            /*
             * Select block from current state.
             */
            uint32_t block =
                select_memory_block(
                    state,
                    memory_blocks,
                    pass * accesses + access,
                    access % 8
                );


            /*
             * Mix selected block.
             */
            mix_memory_block(
                state,
                memory,
                block
            );


            /*
             * Additional state feedback.
             */
            mix_state(
                state,
                block
            );
        }
    }
}


/*
 * ============================================
 * FINALIZATION
 * ============================================
 */

static void finalize(
    uint32_t state[8],
    uint8_t output[
        DEADHASH_OUTPUT_SIZE
    ]
)
{
    /*
     * Final avalanche.
     */
    for (
        int round = 0;
        round < 16;
        round++
    )
    {
        for (
            int i = 0;
            i < 8;
            i++
        )
        {
            uint32_t a =
                state[i];


            uint32_t b =
                state[
                    (i + 1) % 8
                ];


            uint32_t c =
                state[
                    (i + 3) % 8
                ];


            a ^= b;

            a += c;


            a =
                rotate_left(
                    a,
                    (unsigned int)(
                        5 + (i * 3)
                    )
                );


            a *=
                0x9E3779B1;


            a ^= a >> 16;


            state[i] = a;
        }
    }


    /*
     * Convert state into output.
     */
    for (
        int i = 0;
        i < 8;
        i++
    )
    {
        uint32_t value =
            state[i];


        output[
            i * 4
        ] =
            (uint8_t)value;


        output[
            i * 4 + 1
        ] =
            (uint8_t)(
                value >> 8
            );


        output[
            i * 4 + 2
        ] =
            (uint8_t)(
                value >> 16
            );


        output[
            i * 4 + 3
        ] =
            (uint8_t)(
                value >> 24
            );
    }
}


/*
 * ============================================
 * MAIN HASH FUNCTION
 * ============================================
 */

int deadhash_with_params(
    const char *password,
    const uint8_t *salt,
    uint32_t rounds,
    uint32_t memory_blocks,
    uint8_t *output
)
{
    /*
     * Validate rounds.
     */
    if (
        rounds <
        DEADHASH_MIN_ROUNDS ||
        rounds >
        DEADHASH_MAX_ROUNDS
    )
    {
        return 0;
    }


    /*
     * Validate memory.
     */
    if (
        memory_blocks <
        DEADHASH_MIN_MEMORY_BLOCKS ||
        memory_blocks >
        DEADHASH_MAX_MEMORY_BLOCKS
    )
    {
        return 0;
    }


    /*
     * ========================================
     * INITIAL STATE
     * ========================================
     */

    uint32_t state[8] = {

        0x243F6A88,
        0x85A308D3,
        0x13198A2E,
        0x03707344,

        0xA4093822,
        0x299F31D0,
        0x082EFA98,
        0xEC4E6C89
    };


    /*
     * ========================================
     * PASSWORD
     * ========================================
     */

    size_t password_length =
        strlen(password);


    for (
        size_t i = 0;
        i < password_length;
        i++
    )
    {
        mix_state(
            state,
            (uint8_t)password[i]
        );
    }


    /*
     * ========================================
     * SALT
     * ========================================
     */

    for (
        size_t i = 0;
        i < DEADHASH_SALT_SIZE;
        i++
    )
    {
        mix_state(
            state,
            salt[i]
        );
    }


    /*
     * ========================================
     * MEMORY ALLOCATION
     * ========================================
     */

    size_t memory_size =
        (size_t)memory_blocks *
        DEADHASH_BLOCK_SIZE;


    uint8_t *memory =
        malloc(memory_size);


    if (memory == NULL) {
        return 0;
    }


    /*
     * ========================================
     * INITIAL MEMORY FILL
     * ========================================
     */

    fill_memory(
        state,
        memory,
        memory_blocks
    );


    /*
     * ========================================
     * CPU STRETCHING
     * ========================================
     */

    for (
        uint32_t round = 0;
        round < rounds;
        round++
    )
    {
        /*
         * Mix all state words.
         */
        for (
            int i = 0;
            i < 8;
            i++
        )
        {
            mix_state(
                state,
                state[
                    (i + 1) % 8
                ]
            );
        }
    }


    /*
     * ========================================
     * MEMORY-HARD MIXING
     * ========================================
     *
     * The memory access pattern depends
     * on the evolving state.
     */

    memory_hard_mix(
        state,
        memory,
        memory_blocks,
        rounds
    );


    /*
     * ========================================
     * FINALIZATION
     * ========================================
     */

    finalize(
        state,
        output
    );


    /*
     * ========================================
     * SECURE MEMORY CLEANUP
     * ========================================
     */

    memset(
        memory,
        0,
        memory_size
    );


    free(memory);


    return 1;
}


/*
 * ============================================
 * DEFAULT HASH
 * ============================================
 */

void deadhash(
    const char *password,
    const uint8_t *salt,
    uint8_t *output
)
{
    deadhash_with_params(
        password,
        salt,
        DEADHASH_ROUNDS,
        DEADHASH_MEMORY_BLOCKS,
        output
    );
}


/*
 * ============================================
 * CONSTANT-TIME COMPARISON
 * ============================================
 */

static int constant_time_compare(
    const uint8_t *a,
    const uint8_t *b,
    size_t length
)
{
    uint8_t difference = 0;


    for (
        size_t i = 0;
        i < length;
        i++
    )
    {
        difference |=
            a[i] ^ b[i];
    }


    return difference == 0;
}


/*
 * ============================================
 * DEFAULT VERIFICATION
 * ============================================
 */

int deadhash_verify(
    const char *password,
    const uint8_t *salt,
    const uint8_t *stored_hash
)
{
    uint8_t calculated_hash[
        DEADHASH_OUTPUT_SIZE
    ];


    if (!deadhash_with_params(
            password,
            salt,
            DEADHASH_ROUNDS,
            DEADHASH_MEMORY_BLOCKS,
            calculated_hash
        ))
    {
        return 0;
    }


    return constant_time_compare(
        calculated_hash,
        stored_hash,
        DEADHASH_OUTPUT_SIZE
    );
}


/*
 * ============================================
 * HEX ENCODING
 * ============================================
 */

void deadhash_hex_encode(
    const uint8_t *input,
    size_t input_length,
    char *output
)
{
    static const char hex[] =
        "0123456789abcdef";


    for (
        size_t i = 0;
        i < input_length;
        i++
    )
    {
        output[
            i * 2
        ] =
            hex[
                input[i] >> 4
            ];


        output[
            i * 2 + 1
        ] =
            hex[
                input[i] & 0x0F
            ];
    }


    output[
        input_length * 2
    ] =
        '\0';
}


/*
 * ============================================
 * HEX VALUE
 * ============================================
 */

static int hex_value(char c)
{
    if (
        c >= '0' &&
        c <= '9'
    )
    {
        return c - '0';
    }


    if (
        c >= 'a' &&
        c <= 'f'
    )
    {
        return c - 'a' + 10;
    }


    if (
        c >= 'A' &&
        c <= 'F'
    )
    {
        return c - 'A' + 10;
    }


    return -1;
}


/*
 * ============================================
 * HEX DECODING
 * ============================================
 */

static int deadhash_hex_decode(
    const char *input,
    uint8_t *output,
    size_t output_length
)
{
    size_t input_length =
        strlen(input);


    if (
        input_length !=
        output_length * 2
    )
    {
        return 0;
    }


    for (
        size_t i = 0;
        i < output_length;
        i++
    )
    {
        int high =
            hex_value(
                input[i * 2]
            );


        int low =
            hex_value(
                input[i * 2 + 1]
            );


        if (
            high < 0 ||
            low < 0
        )
        {
            return 0;
        }


        output[i] =
            (uint8_t)(
                (high << 4) |
                low
            );
    }


    return 1;
}


/*
 * ============================================
 * RECORD VERIFICATION
 * ============================================
 */

int deadhash_verify_record(
    const char *password,
    const char *record
)
{
    char buffer[
        DEADHASH_MAX_STRING_SIZE
    ];


    /*
     * strtok() modifies its input,
     * so work on a copy.
     */
    if (
        strlen(record) >=
        sizeof(buffer)
    )
    {
        return 0;
    }


    strcpy(
        buffer,
        record
    );


    /*
     * Parse:
     *
     * DEADHASH
     * VERSION
     * ROUNDS
     * MEMORY
     * SALT
     * HASH
     */

    char *algorithm =
        strtok(
            buffer,
            "$"
        );


    char *version_string =
        strtok(
            NULL,
            "$"
        );


    char *rounds_string =
        strtok(
            NULL,
            "$"
        );


    char *memory_string =
        strtok(
            NULL,
            "$"
        );


    char *salt_string =
        strtok(
            NULL,
            "$"
        );


    char *hash_string =
        strtok(
            NULL,
            "$"
        );


    char *extra =
        strtok(
            NULL,
            "$"
        );


    /*
     * Check fields.
     */
    if (
        algorithm == NULL ||
        version_string == NULL ||
        rounds_string == NULL ||
        memory_string == NULL ||
        salt_string == NULL ||
        hash_string == NULL ||
        extra != NULL
    )
    {
        return 0;
    }


    /*
     * Check algorithm name.
     */
    if (
        strcmp(
            algorithm,
            "DEADHASH"
        ) != 0
    )
    {
        return 0;
    }


    /*
     * ========================================
     * VERSION
     * ========================================
     */

    char *end;


    long version =
        strtol(
            version_string,
            &end,
            10
        );


    if (
        *end != '\0' ||
        version != DEADHASH_VERSION
    )
    {
        return 0;
    }


    /*
     * ========================================
     * ROUNDS
     * ========================================
     */

    long rounds =
        strtol(
            rounds_string,
            &end,
            10
        );


    if (
        *end != '\0' ||
        rounds <
            DEADHASH_MIN_ROUNDS ||
        rounds >
            DEADHASH_MAX_ROUNDS
    )
    {
        return 0;
    }


    /*
     * ========================================
     * MEMORY BLOCKS
     * ========================================
     */

    long memory_blocks =
        strtol(
            memory_string,
            &end,
            10
        );


    if (
        *end != '\0' ||
        memory_blocks <
            DEADHASH_MIN_MEMORY_BLOCKS ||
        memory_blocks >
            DEADHASH_MAX_MEMORY_BLOCKS
    )
    {
        return 0;
    }


    /*
     * ========================================
     * SALT
     * ========================================
     */

    uint8_t salt[
        DEADHASH_SALT_SIZE
    ];


    if (!deadhash_hex_decode(
            salt_string,
            salt,
            DEADHASH_SALT_SIZE
        ))
    {
        return 0;
    }


    /*
     * ========================================
     * STORED HASH
     * ========================================
     */

    uint8_t stored_hash[
        DEADHASH_OUTPUT_SIZE
    ];


    if (!deadhash_hex_decode(
            hash_string,
            stored_hash,
            DEADHASH_OUTPUT_SIZE
        ))
    {
        return 0;
    }


    /*
     * ========================================
     * CALCULATE HASH
     * ========================================
     */

    uint8_t calculated_hash[
        DEADHASH_OUTPUT_SIZE
    ];


    if (!deadhash_with_params(
            password,
            salt,
            (uint32_t)rounds,
            (uint32_t)memory_blocks,
            calculated_hash
        ))
    {
        return 0;
    }


    /*
     * ========================================
     * CONSTANT-TIME COMPARE
     * ========================================
     */

    return constant_time_compare(
        calculated_hash,
        stored_hash,
        DEADHASH_OUTPUT_SIZE
    );
}


