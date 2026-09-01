
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "deadhash.h"


/*
 * ============================================================
 * PASSWORD INPUT
 * ============================================================
 *
 * Linux terminal password input.
 *
 * Characters are not displayed while typing.
 *
 * ============================================================
 */

#include <termios.h>
#include <unistd.h>


static int read_password(
    char *password,
    size_t size
)
{
    struct termios old_terminal;
    struct termios new_terminal;


    if (
        password == NULL ||
        size == 0
    )
    {
        return 0;
    }


    /*
     * Save terminal settings.
     */
    if (
        tcgetattr(
            STDIN_FILENO,
            &old_terminal
        ) != 0
    )
    {
        return 0;
    }


    new_terminal =
        old_terminal;


    /*
     * Disable echo.
     */
    new_terminal.c_lflag &=
        ~(ECHO);


    if (
        tcsetattr(
            STDIN_FILENO,
            TCSAFLUSH,
            &new_terminal
        ) != 0
    )
    {
        return 0;
    }


    printf(
        "Password: "
    );


    fflush(
        stdout
    );


    /*
     * Read password.
     */
    if (
        fgets(
            password,
            size,
            stdin
        ) == NULL
    )
    {
        /*
         * Restore terminal even
         * when reading fails.
         */
        tcsetattr(
            STDIN_FILENO,
            TCSAFLUSH,
            &old_terminal
        );


        return 0;
    }


    /*
     * Restore terminal.
     */
    tcsetattr(
        STDIN_FILENO,
        TCSAFLUSH,
        &old_terminal
    );


    /*
     * Remove newline.
     */
    password[
        strcspn(
            password,
            "\n"
        )
    ] = '\0';


    printf(
        "\n"
    );


    return 1;
}


/*
 * ============================================================
 * CREATE PASSWORD HASH
 * ============================================================
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


    char record[
        DEADHASH_MAX_STRING_SIZE
    ];


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
     * Generate salt.
     */
    if (!deadhash_generate_salt(
            salt,
            sizeof(salt)
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


    deadhash_hex_encode(
        salt,
        sizeof(salt),
        salt_hex
    );


    deadhash_hex_encode(
        hash,
        sizeof(hash),
        hash_hex
    );


    snprintf(
        record,
        sizeof(record),
        "DEADHASH$%d$%d$%d$%s$%s",

        DEADHASH_VERSION,

        DEADHASH_ROUNDS,

        DEADHASH_MEMORY_BLOCKS,

        salt_hex,

        hash_hex
    );


    printf(
        "\n========================================\n"
    );


    printf(
        "PASSWORD RECORD\n"
    );


    printf(
        "========================================\n"
    );


    printf(
        "%s\n",
        record
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
 * ============================================================
 * VERIFY PASSWORD
 * ============================================================
 */

static void verify_password(void)
{
    char password[256];


    char record[
        DEADHASH_MAX_STRING_SIZE
    ];


    printf(
        "\nStored DEADHASH record:\n"
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


    record[
        strcspn(
            record,
            "\n"
        )
    ] = '\0';


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


    memset(
        password,
        0,
        sizeof(password)
    );
}


/*
 * ============================================================
 * ENCRYPT FILE
 * ============================================================
 */

static void encrypt_file(void)
{
    char input_path[512];

    char output_path[512];

    char password[256];


    printf(
        "\nInput file: "
    );


    if (
        fgets(
            input_path,
            sizeof(input_path),
            stdin
        ) == NULL
    )
    {
        return;
    }


    input_path[
        strcspn(
            input_path,
            "\n"
        )
    ] = '\0';


    printf(
        "Output encrypted file: "
    );


    if (
        fgets(
            output_path,
            sizeof(output_path),
            stdin
        ) == NULL
    )
    {
        return;
    }


    output_path[
        strcspn(
            output_path,
            "\n"
        )
    ] = '\0';


    /*
     * Password is hidden.
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


    printf(
        "\nEncrypting...\n"
    );


    if (
        deadhash_encrypt_file(
            input_path,
            output_path,
            password
        )
    )
    {
        printf(
            "\nFile encrypted successfully.\n"
        );


        printf(
            "Output: %s\n",
            output_path
        );
    }
    else
    {
        printf(
            "\nFile encryption failed.\n"
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
 * ============================================================
 * DECRYPT FILE
 * ============================================================
 */

static void decrypt_file(void)
{
    char input_path[512];

    char output_path[512];

    char password[256];


    printf(
        "\nEncrypted file: "
    );


    if (
        fgets(
            input_path,
            sizeof(input_path),
            stdin
        ) == NULL
    )
    {
        return;
    }


    input_path[
        strcspn(
            input_path,
            "\n"
        )
    ] = '\0';


    printf(
        "Output decrypted file: "
    );


    if (
        fgets(
            output_path,
            sizeof(output_path),
            stdin
        ) == NULL
    )
    {
        return;
    }


    output_path[
        strcspn(
            output_path,
            "\n"
        )
    ] = '\0';


    /*
     * Hidden password input.
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


    printf(
        "\nDecrypting...\n"
    );


    if (
        deadhash_decrypt_file(
            input_path,
            output_path,
            password
        )
    )
    {
        printf(
            "\nFile decrypted successfully.\n"
        );


        printf(
            "Output: %s\n",
            output_path
        );
    }
    else
    {
        printf(
            "\nFile decryption failed.\n"
        );


        printf(
            "Check the password and encrypted file.\n"
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
 * ============================================================
 * MAIN MENU
 * ============================================================
 */

int main(void)
{
    char choice[16];


    while (1)
    {
        printf(
            "\n"
        );


        printf(
            "================================\n"
        );


        printf(
            "          DEADHASH\n"
        );


        printf(
            "================================\n"
        );


        printf(
            "1. Hash password\n"
        );


        printf(
            "2. Verify password\n"
        );


        printf(
            "3. Encrypt file\n"
        );


        printf(
            "4. Decrypt file\n"
        );


        printf(
            "5. Exit\n"
        );


        printf(
            "================================\n"
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


        switch (
            choice[0]
        )
        {
            case '1':

                hash_password();

                break;


            case '2':

                verify_password();

                break;


            case '3':

                encrypt_file();

                break;


            case '4':

                decrypt_file();

                break;


            case '5':

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


