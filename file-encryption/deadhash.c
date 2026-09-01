
#include "deadhash.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
 * ============================================================
 * ROTATE LEFT
 * ============================================================
 */

static uint32_t rotate_left(
    uint32_t value,
    unsigned int amount
)
{
    return
        (value << amount) |
        (value >> (32 - amount));
}


/*
 * ============================================================
 * STATE MIXING
 * ============================================================
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
 * ============================================================
 * RANDOM SALT
 * ============================================================
 */

int deadhash_generate_salt(
    uint8_t *salt,
    size_t length
)
{
    if (
        salt == NULL ||
        length == 0
    )
    {
        return 0;
    }


    FILE *random_source =
        fopen(
            "/dev/urandom",
            "rb"
        );


    if (
        random_source == NULL
    )
    {
        return 0;
    }


    size_t bytes_read =
        fread(
            salt,
            1,
            length,
            random_source
        );


    fclose(
        random_source
    );


    return
        bytes_read == length;
}


/*
 * ============================================================
 * MEMORY INITIALIZATION
 * ============================================================
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
            uint32_t value =
                state[
                    (i + block) % 8
                ];


            value ^= block;


            value ^=
                (uint32_t)i << 8;


            value =
                rotate_left(
                    value,
                    (unsigned int)(
                        (i % 31) + 1
                    )
                );


            value *=
                0x9E3779B1;


            memory[
                (size_t)block *
                DEADHASH_BLOCK_SIZE +
                i
            ] =
                (uint8_t)value;


            state[
                i % 8
            ] ^= value;
        }
    }
}


/*
 * ============================================================
 * STATE-DEPENDENT MEMORY SELECTION
 * ============================================================
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


    return
        value % memory_blocks;
}


/*
 * ============================================================
 * MIX MEMORY BLOCK
 * ============================================================
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
        uint32_t value =
            memory[
                offset + i
            ];


        mix_state(
            state,
            value
        );


        uint32_t generated =
            state[
                i % 8
            ];


        generated ^= value;


        generated =
            rotate_left(
                generated,
                (unsigned int)(
                    (i % 13) + 1
                )
            );


        memory[
            offset + i
        ] =
            (uint8_t)generated;
    }
}


/*
 * ============================================================
 * MEMORY-HARD MIXING
 * ============================================================
 */

static void memory_hard_mix(
    uint32_t state[8],
    uint8_t *memory,
    uint32_t memory_blocks,
    uint32_t rounds
)
{
    uint32_t memory_passes =
        rounds / 1000;


    if (
        memory_passes < 1
    )
    {
        memory_passes = 1;
    }


    if (
        memory_passes > 1024
    )
    {
        memory_passes = 1024;
    }


    for (
        uint32_t pass = 0;
        pass < memory_passes;
        pass++
    )
    {
        for (
            uint32_t access = 0;
            access < memory_blocks;
            access++
        )
        {
            uint32_t block =
                select_memory_block(
                    state,
                    memory_blocks,
                    pass * memory_blocks + access,
                    access % 8
                );


            mix_memory_block(
                state,
                memory,
                block
            );


            mix_state(
                state,
                block
            );
        }
    }
}


/*
 * ============================================================
 * FINALIZATION
 * ============================================================
 */

static void finalize(
    uint32_t state[8],
    uint8_t output[
        DEADHASH_OUTPUT_SIZE
    ]
)
{
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
 * ============================================================
 * DEADHASH CORE
 * ============================================================
 */

int deadhash_with_params(
    const char *password,
    const uint8_t *salt,
    uint32_t rounds,
    uint32_t memory_blocks,
    uint8_t *output
)
{
    if (
        password == NULL ||
        salt == NULL ||
        output == NULL
    )
    {
        return 0;
    }


    if (
        rounds <
        DEADHASH_MIN_ROUNDS ||
        rounds >
        DEADHASH_MAX_ROUNDS
    )
    {
        return 0;
    }


    if (
        memory_blocks <
        DEADHASH_MIN_MEMORY_BLOCKS ||
        memory_blocks >
        DEADHASH_MAX_MEMORY_BLOCKS
    )
    {
        return 0;
    }


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
     * Password.
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
     * Salt.
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
     * Memory.
     */
    size_t memory_size =
        (size_t)memory_blocks *
        DEADHASH_BLOCK_SIZE;


    uint8_t *memory =
        malloc(
            memory_size
        );


    if (
        memory == NULL
    )
    {
        return 0;
    }


    fill_memory(
        state,
        memory,
        memory_blocks
    );


    /*
     * CPU stretching.
     */
    for (
        uint32_t round = 0;
        round < rounds;
        round++
    )
    {
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
     * Memory-hard phase.
     */
    memory_hard_mix(
        state,
        memory,
        memory_blocks,
        rounds
    );


    /*
     * Final hash.
     */
    finalize(
        state,
        output
    );


    /*
     * Clear memory.
     */
    memset(
        memory,
        0,
        memory_size
    );


    free(
        memory
    );


    /*
     * Clear internal state.
     */
    memset(
        state,
        0,
        sizeof(state)
    );


    return 1;
}


/*
 * ============================================================
 * DEFAULT DEADHASH
 * ============================================================
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
 * ============================================================
 * CONSTANT-TIME COMPARISON
 * ============================================================
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
 * ============================================================
 * HEX ENCODING
 * ============================================================
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
 * ============================================================
 * HEX VALUE
 * ============================================================
 */

static int hex_value(
    char c
)
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
 * ============================================================
 * HEX DECODING
 * ============================================================
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
                input[
                    i * 2
                ]
            );


        int low =
            hex_value(
                input[
                    i * 2 + 1
                ]
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
 * ============================================================
 * HASH VERIFICATION
 * ============================================================
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
 * ============================================================
 * RECORD VERIFICATION
 * ============================================================
 */

int deadhash_verify_record(
    const char *password,
    const char *record
)
{
    char buffer[
        DEADHASH_MAX_STRING_SIZE
    ];


    if (
        password == NULL ||
        record == NULL
    )
    {
        return 0;
    }


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


    if (
        strcmp(
            algorithm,
            "DEADHASH"
        ) != 0
    )
    {
        return 0;
    }


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


    return constant_time_compare(
        calculated_hash,
        stored_hash,
        DEADHASH_OUTPUT_SIZE
    );
}


/*
 * ============================================================
 * DERIVE FILE KEY
 * ============================================================
 *
 * We derive the encryption key by running DEADHASH
 * with the file salt.
 *
 * EDUCATIONAL ONLY.
 * ============================================================
 */

static int derive_file_key(
    const char *password,
    const uint8_t salt[
        DEADHASH_FILE_SALT_SIZE
    ],
    uint8_t key[
        DEADHASH_KEY_SIZE
    ]
)
{
    return deadhash_with_params(
        password,
        salt,
        DEADHASH_ROUNDS,
        DEADHASH_MEMORY_BLOCKS,
        key
    );
}


/*
 * ============================================================
 * NONCE + KEY STREAM GENERATOR
 * ============================================================
 *
 * This produces a pseudo-random byte stream.
 *
 * The plaintext is XORed with this stream.
 *
 * IMPORTANT:
 *
 * This is an educational stream cipher.
 *
 * It is NOT a secure replacement for AES-GCM
 * or XChaCha20-Poly1305.
 *
 * ============================================================
 */

static void generate_keystream(
    const uint8_t key[
        DEADHASH_KEY_SIZE
    ],
    const uint8_t nonce[
        DEADHASH_NONCE_SIZE
    ],
    uint64_t counter,
    uint8_t output[
        DEADHASH_OUTPUT_SIZE
    ]
)
{
    uint8_t salt[
        DEADHASH_SALT_SIZE
    ];


    /*
     * Combine nonce and counter
     * into a temporary salt.
     */
    for (
        size_t i = 0;
        i < DEADHASH_SALT_SIZE;
        i++
    )
    {
        salt[i] =
            nonce[
                i % DEADHASH_NONCE_SIZE
            ];


        salt[i] ^=
            (uint8_t)(
                counter >>
                ((i % 8) * 8)
            );
    }


    /*
     * Use the key as password material.
     *
     * We can't safely pass arbitrary binary
     * key data to strlen(), so construct a
     * temporary hexadecimal representation.
     */
    char key_hex[
        DEADHASH_KEY_SIZE * 2 + 1
    ];


    deadhash_hex_encode(
        key,
        DEADHASH_KEY_SIZE,
        key_hex
    );


    /*
     * Derive one 32-byte keystream block.
     */
    deadhash_with_params(
        key_hex,
        salt,
        1000,
        64,
        output
    );


    /*
     * Clear temporary key representation.
     */
    memset(
        key_hex,
        0,
        sizeof(key_hex)
    );


    memset(
        salt,
        0,
        sizeof(salt)
    );
}


/*
 * ============================================================
 * EDUCATIONAL STREAM ENCRYPTION
 * ============================================================
 */

static void crypt_buffer(
    uint8_t *buffer,
    size_t length,
    const uint8_t key[
        DEADHASH_KEY_SIZE
    ],
    const uint8_t nonce[
        DEADHASH_NONCE_SIZE
    ],
    uint64_t *counter
)
{
    size_t position = 0;


    while (
        position < length
    )
    {
        uint8_t keystream[
            DEADHASH_OUTPUT_SIZE
        ];


        generate_keystream(
            key,
            nonce,
            *counter,
            keystream
        );


        size_t remaining =
            length - position;


        size_t amount =
            remaining <
            DEADHASH_OUTPUT_SIZE
                ? remaining
                : DEADHASH_OUTPUT_SIZE;


        for (
            size_t i = 0;
            i < amount;
            i++
        )
        {
            buffer[
                position + i
            ] ^=
                keystream[i];
        }


        position += amount;


        (*counter)++;


        memset(
            keystream,
            0,
            sizeof(keystream)
        );
    }
}


/*
 * ============================================================
 * FILE HEADER
 * ============================================================
 *
 * File layout:
 *
 *     MAGIC
 *     VERSION
 *     SALT
 *     NONCE
 *     FILE DATA
 *
 * ============================================================
 */

#define DEADHASH_FILE_MAGIC "DHF1"

#define DEADHASH_FILE_MAGIC_SIZE 4


/*
 * ============================================================
 * ENCRYPT FILE
 * ============================================================
 */

int deadhash_encrypt_file(
    const char *input_path,
    const char *output_path,
    const char *password
)
{
    if (
        input_path == NULL ||
        output_path == NULL ||
        password == NULL
    )
    {
        return 0;
    }


    /*
     * Open input.
     */
    FILE *input =
        fopen(
            input_path,
            "rb"
        );


    if (
        input == NULL
    )
    {
        return 0;
    }


    /*
     * Open output.
     */
    FILE *output =
        fopen(
            output_path,
            "wb"
        );


    if (
        output == NULL
    )
    {
        fclose(input);

        return 0;
    }


    /*
     * Generate salt.
     */
    uint8_t salt[
        DEADHASH_FILE_SALT_SIZE
    ];


    if (!deadhash_generate_salt(
            salt,
            sizeof(salt)
        ))
    {
        fclose(input);
        fclose(output);

        return 0;
    }


    /*
     * Generate nonce.
     */
    uint8_t nonce[
        DEADHASH_NONCE_SIZE
    ];


    if (!deadhash_generate_salt(
            nonce,
            sizeof(nonce)
        ))
    {
        fclose(input);
        fclose(output);

        return 0;
    }


    /*
     * Derive encryption key.
     */
    uint8_t key[
        DEADHASH_KEY_SIZE
    ];


    if (!derive_file_key(
            password,
            salt,
            key
        ))
    {
        fclose(input);
        fclose(output);

        return 0;
    }


    /*
     * Write magic.
     */
    if (
        fwrite(
            DEADHASH_FILE_MAGIC,
            1,
            DEADHASH_FILE_MAGIC_SIZE,
            output
        ) != DEADHASH_FILE_MAGIC_SIZE
    )
    {
        fclose(input);
        fclose(output);

        return 0;
    }


    /*
     * Write version.
     */
    uint8_t version =
        DEADHASH_VERSION;


    if (
        fwrite(
            &version,
            1,
            1,
            output
        ) != 1
    )
    {
        fclose(input);
        fclose(output);

        return 0;
    }


    /*
     * Write salt.
     */
    if (
        fwrite(
            salt,
            1,
            sizeof(salt),
            output
        ) != sizeof(salt)
    )
    {
        fclose(input);
        fclose(output);

        return 0;
    }


    /*
     * Write nonce.
     */
    if (
        fwrite(
            nonce,
            1,
            sizeof(nonce),
            output
        ) != sizeof(nonce)
    )
    {
        fclose(input);
        fclose(output);

        return 0;
    }


    /*
     * Encrypt file contents.
     */
    uint8_t buffer[
        DEADHASH_FILE_BUFFER_SIZE
    ];


    uint64_t counter = 0;


    while (1)
    {
        size_t bytes_read =
            fread(
                buffer,
                1,
                sizeof(buffer),
                input
            );


        if (
            bytes_read > 0
        )
        {
            crypt_buffer(
                buffer,
                bytes_read,
                key,
                nonce,
                &counter
            );


            if (
                fwrite(
                    buffer,
                    1,
                    bytes_read,
                    output
                ) != bytes_read
            )
            {
                fclose(input);
                fclose(output);

                memset(
                    key,
                    0,
                    sizeof(key)
                );

                return 0;
            }
        }


        if (
            bytes_read <
            sizeof(buffer)
        )
        {
            if (
                ferror(input)
            )
            {
                fclose(input);
                fclose(output);

                memset(
                    key,
                    0,
                    sizeof(key)
                );

                return 0;
            }


            break;
        }
    }


    /*
     * Cleanup.
     */
    memset(
        key,
        0,
        sizeof(key)
    );


    memset(
        salt,
        0,
        sizeof(salt)
    );


    memset(
        nonce,
        0,
        sizeof(nonce)
    );


    fclose(input);

    fclose(output);


    return 1;
}


/*
 * ============================================================
 * DECRYPT FILE
 * ============================================================
 */

int deadhash_decrypt_file(
    const char *input_path,
    const char *output_path,
    const char *password
)
{
    if (
        input_path == NULL ||
        output_path == NULL ||
        password == NULL
    )
    {
        return 0;
    }


    /*
     * Open encrypted file.
     */
    FILE *input =
        fopen(
            input_path,
            "rb"
        );


    if (
        input == NULL
    )
    {
        return 0;
    }


    /*
     * Read magic.
     */
    char magic[
        DEADHASH_FILE_MAGIC_SIZE
    ];


    if (
        fread(
            magic,
            1,
            sizeof(magic),
            input
        ) != sizeof(magic)
    )
    {
        fclose(input);

        return 0;
    }


    if (
        memcmp(
            magic,
            DEADHASH_FILE_MAGIC,
            DEADHASH_FILE_MAGIC_SIZE
        ) != 0
    )
    {
        fclose(input);

        return 0;
    }


    /*
     * Read version.
     */
    uint8_t version;


    if (
        fread(
            &version,
            1,
            1,
            input
        ) != 1
    )
    {
        fclose(input);

        return 0;
    }


    if (
        version != DEADHASH_VERSION
    )
    {
        fclose(input);

        return 0;
    }


    /*
     * Read salt.
     */
    uint8_t salt[
        DEADHASH_FILE_SALT_SIZE
    ];


    if (
        fread(
            salt,
            1,
            sizeof(salt),
            input
        ) != sizeof(salt)
    )
    {
        fclose(input);

        return 0;
    }


    /*
     * Read nonce.
     */
    uint8_t nonce[
        DEADHASH_NONCE_SIZE
    ];


    if (
        fread(
            nonce,
            1,
            sizeof(nonce),
            input
        ) != sizeof(nonce)
    )
    {
        fclose(input);

        return 0;
    }


    /*
     * Derive key using supplied password.
     */
    uint8_t key[
        DEADHASH_KEY_SIZE
    ];


    if (!derive_file_key(
            password,
            salt,
            key
        ))
    {
        fclose(input);

        return 0;
    }


    /*
     * Open output.
     */
    FILE *output =
        fopen(
            output_path,
            "wb"
        );


    if (
        output == NULL
    )
    {
        fclose(input);

        memset(
            key,
            0,
            sizeof(key)
        );

        return 0;
    }


    /*
     * Decrypt.
     */
    uint8_t buffer[
        DEADHASH_FILE_BUFFER_SIZE
    ];


    uint64_t counter = 0;


    while (1)
    {
        size_t bytes_read =
            fread(
                buffer,
                1,
                sizeof(buffer),
                input
            );


        if (
            bytes_read > 0
        )
        {
            /*
             * XOR is symmetrical:
             *
             * encrypted XOR key = plaintext
             *
             * plaintext XOR key = encrypted
             */
            crypt_buffer(
                buffer,
                bytes_read,
                key,
                nonce,
                &counter
            );


            if (
                fwrite(
                    buffer,
                    1,
                    bytes_read,
                    output
                ) != bytes_read
            )
            {
                fclose(input);
                fclose(output);

                memset(
                    key,
                    0,
                    sizeof(key)
                );

                return 0;
            }
        }


        if (
            bytes_read <
            sizeof(buffer)
        )
        {
            if (
                ferror(input)
            )
            {
                fclose(input);
                fclose(output);

                memset(
                    key,
                    0,
                    sizeof(key)
                );

                return 0;
            }


            break;
        }
    }


    /*
     * Cleanup.
     */
    memset(
        key,
        0,
        sizeof(key)
    );


    memset(
        salt,
        0,
        sizeof(salt)
    );


    memset(
        nonce,
        0,
        sizeof(nonce)
    );


    fclose(input);

    fclose(output);


    return 1;
}

