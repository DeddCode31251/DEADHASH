#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <termios.h>
#include <unistd.h>

#include "deadhash.h"


/*
 * ============================================
 * READ PASSWORD
 * ============================================
 */


static int read_password(char *password, size_t size)
{
    struct termios old_terminal;
    struct termios new_terminal;

    if (tcgetattr(STDIN_FILENO, &old_terminal) != 0)
        return 0;

    new_terminal = old_terminal;

    /* Disable displaying typed characters */
    new_terminal.c_lflag &= ~(ECHO);

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_terminal) != 0)
        return 0;

    printf("Enter password: ");
    fflush(stdout);

    if (fgets(password, size, stdin) == NULL)
    {
        /* Always restore terminal */
        tcsetattr(
            STDIN_FILENO,
            TCSAFLUSH,
            &old_terminal
        );

        return 0;
    }

    /* Restore terminal */
    tcsetattr(
        STDIN_FILENO,
        TCSAFLUSH,
        &old_terminal
    );

    /* Remove newline */
    password[
        strcspn(password, "\n")
    ] = '\0';

    printf("\n");

    return 1;
}


/*
 * ============================================
 * CREATE PASSWORD
 * ============================================
 */

static void hash_password(void)
{
    char password[256];


    uint8_t salt[
        DEADHASH_SALT_SIZE
    ];


    uint8_t hash[
        DEADHASH_OUTPUT_SIZE
    ];


    char salt_hex[
        DEADHASH_SALT_SIZE * 2 + 1
    ];


    char hash_hex[
        DEADHASH_OUTPUT_SIZE * 2 + 1
    ];


    char stored_record[
        DEADHASH_MAX_STRING_SIZE
    ];


    /*
     * Get password.
     */
    if (!read_password(
            password,
            sizeof(password)
        ))
    {
        printf(
            "Failed to read password.\n"
        );

        return;
    }


    /*
     * Generate random salt.
     */
    if (!deadhash_generate_salt(
            salt,
            DEADHASH_SALT_SIZE
        ))
    {
        printf(
            "Failed to generate salt.\n"
        );


        memset(
            password,
            0,
            sizeof(password)
        );


        return;
    }


    /*
     * Hash.
     */
    printf(
        "\nHashing...\n"
    );


    if (!deadhash_with_params(
            password,
            salt,
            DEADHASH_ROUNDS,
            DEADHASH_MEMORY_BLOCKS,
            hash
        ))
    {
        printf(
            "Hashing failed.\n"
        );


        memset(
            password,
            0,
            sizeof(password)
        );


        return;
    }


    /*
     * Encode salt.
     */
    deadhash_hex_encode(
        salt,
        DEADHASH_SALT_SIZE,
        salt_hex
    );


    /*
     * Encode hash.
     */
    deadhash_hex_encode(
        hash,
        DEADHASH_OUTPUT_SIZE,
        hash_hex
    );


    /*
     * Create stored record.
     */
    int written =
        snprintf(
            stored_record,
            sizeof(stored_record),

            "DEADHASH$%d$%d$%d$%s$%s",

            DEADHASH_VERSION,

            DEADHASH_ROUNDS,

            DEADHASH_MEMORY_BLOCKS,

            salt_hex,

            hash_hex
        );


    if (
        written < 0 ||
        (size_t)written >=
            sizeof(stored_record)
    )
    {
        printf(
            "Failed to create record.\n"
        );


        memset(
            password,
            0,
            sizeof(password)
        );


        return;
    }


    /*
     * Display record.
     */
    printf(
        "\n========================================\n"
    );


    printf(
        "       DEADHASH PASSWORD RECORD\n"
    );


    printf(
        "========================================\n"
    );


    printf(
        "%s\n",
        stored_record
    );


    printf(
        "========================================\n"
    );


    /*
     * Clear password.
     */
    memset(
        password,
        0,
        sizeof(password)
    );
}


/*
 * ============================================
 * VERIFY PASSWORD
 * ============================================
 */

static void verify_password(void)
{
    char password[256];


    char record[
        DEADHASH_MAX_STRING_SIZE
    ];


    /*
     * Read stored record.
     */
    printf(
        "\nEnter stored DEADHASH record:\n"
    );


    if (
        fgets(
            record,
            sizeof(record),
            stdin
        ) == NULL
    )
    {
        return;
    }


    /*
     * Remove newline.
     */
    record[
        strcspn(
            record,
            "\n"
        )
    ] = '\0';


    /*
     * Read password.
     */
    if (!read_password(
            password,
            sizeof(password)
        ))
    {
        printf(
            "Failed to read password.\n"
        );

        return;
    }


    /*
     * Verify.
     */
    printf(
        "\nVerifying...\n"
    );


    if (
        deadhash_verify_record(
            password,
            record
        )
    )
    {
        printf(
            "\nPASSWORD CORRECT\n"
        );
    }
    else
    {
        printf(
            "\nPASSWORD INCORRECT\n"
        );
    }


    /*
     * Clear password.
     */
    memset(
        password,
        0,
        sizeof(password)
    );
}


/*
 * ============================================
 * MAIN
 * ============================================
 */

int main(void)
{
    char choice[16];


    while (1)
    {
        printf("\n");


        printf(
            "====================\n"
        );


        printf(
            "      DEADHASH\n"
        );


        printf(
            "====================\n"
        );


        printf(
            "1. Create password\n"
        );


        printf(
            "2. Verify password\n"
        );


        printf(
            "3. Exit\n"
        );


        printf(
            "====================\n"
        );


        printf(
            "Choice: "
        );


        if (
            fgets(
                choice,
                sizeof(choice),
                stdin
            ) == NULL
        )
        {
            break;
        }


        switch (choice[0])
        {
            case '1':

                hash_password();

                break;


            case '2':

                verify_password();

                break;


            case '3':

                printf(
                    "Goodbye.\n"
                );

                return 0;


            default:

                printf(
                    "Invalid choice.\n"
                );

                break;
        }
    }


    return 0;
}


