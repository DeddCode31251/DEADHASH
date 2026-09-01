# DEADHASH

<img width="275" height="189" alt="Screenshot From 2026-09-01 19-19-50" src="https://github.com/user-attachments/assets/56eab1d6-d073-4e60-b855-5143ecb44f94" />


Made by Deadcode (vlogix or DeadIV)

DEADHASH is an educational password hashing and file encryption project written in C.

The project was created to explore how password hashing, salts, password stretching, memory-hard computation, key derivation, secure password input, file formats, and encryption work at a low level.

> **Security Warning**
>
> DEADHASH is an educational cryptography project and has not been professionally cryptographically audited.
>
> The current file-encryption implementation is intended for learning and experimentation only. Do not use it to protect passwords, private files, credentials, financial information, or other sensitive data in production.
>
> For real applications, use established and professionally reviewed cryptographic constructions and libraries.

---

## Features

### Password hashing

DEADHASH includes:

* Random salt generation
* Password hashing
* Configurable computational rounds
* Memory-based computation
* Memory cleanup
* Password verification
* Constant-time hash comparison
* Hexadecimal encoding
* Versioned password records

Example password record:

```text
DEADHASH$1$100000$1024$<salt>$<hash>
```

The record contains the parameters required to reproduce the hash during verification.

---

## File encryption

The educational version also supports:

* Encrypting files
* Decrypting files
* Random file salts
* Random nonces
* Password-derived encryption keys
* Chunk-based file processing
* Binary encrypted files
* Custom encrypted file headers

Example:

```text
secret.txt
    |
    | password
    v
DEADHASH
    |
    v
encryption key
    |
    v
encrypted file
```

---

## Important: Hashing vs Encryption

A password hash and encryption are different things.

### Hashing

Hashing is one-way:

```text
password
   |
   v
   HASH
   |
   v
hash
```

You do not decrypt a password hash.

During login, the application:

```text
Entered password
       |
       v
     HASH
       |
       v
Compare with stored hash
```

If the values match, the password is considered correct.

---

### Encryption

Encryption is reversible when the correct key is available:

```text
plaintext
    |
    | encryption key
    v
ciphertext
    |
    | same key
    v
plaintext
```

This is why file encryption requires a different design from password hashing.

---

# Project Structure

A typical educational version of the project looks like:

```text
DEADHASH/
│
├── deadhash.h
├── deadhash.c
├── main.c
│
├── README.md
├── LICENSE
│
└── deadhash
```

---

# File Descriptions

## `deadhash.h`

The header file contains the public interface of DEADHASH.

It defines:

* Constants
* Hash parameters
* File encryption parameters
* Function declarations

For example:

```c
int deadhash_generate_salt(
    uint8_t *salt,
    size_t length
);
```

The header allows `main.c` to use functions implemented inside `deadhash.c`.

---

## `deadhash.c`

This contains the implementation.

It includes:

* Salt generation
* Password hashing
* Memory mixing
* Password stretching
* Finalization
* Constant-time comparison
* Hex encoding
* Password record verification
* File key derivation
* File encryption
* File decryption

---

## `main.c`

This contains the command-line interface.

It provides the menu:

```text
================================
          DEADHASH
================================
1. Hash password
2. Verify password
3. Encrypt file
4. Decrypt file
5. Exit
================================
Choice:
```

It also provides hidden password input on Linux terminals.

---

# Requirements

DEADHASH requires:

* Linux or another POSIX-compatible operating system
* GCC or another C compiler
* C11 support

The educational version does not require external cryptographic libraries.

---

# Compilation

Compile the project with:

```bash
gcc -Wall -Wextra -std=c11 main.c deadhash.c -o deadhash
```

Then run:

```bash
./deadhash
```

---

# Password Hashing

Select:

```text
1. Hash password
```

The program asks for a password:

```text
Password:
```

The password is not displayed while typing.

DEADHASH generates a random salt and calculates the password hash.

The resulting record looks similar to:

```text
DEADHASH$1$100000$1024$9f2a...$8c71...
```

The exact values will be different every time because the salt is random.

---

# Why Use a Salt?

Without a salt:

```text
password123
     |
     v
   HASH A
```

Every user using the same password could produce the same hash.

With a random salt:

```text
password123 + salt A
        |
        v
      HASH A


password123 + salt B
        |
        v
      HASH B
```

Therefore, the same password can produce different stored records.

The salt does not need to be secret.

---

# Password Verification

Select:

```text
2. Verify password
```

The program asks for the stored DEADHASH record:

```text
Stored DEADHASH record:
```

Then it asks for the password:

```text
Password:
```

DEADHASH extracts the parameters, salt, and stored hash from the record.

It then calculates:

```text
entered password
       +
stored salt
       |
       v
   DEADHASH
       |
       v
calculated hash
```

The calculated hash is compared against the stored hash.

The comparison uses a constant-time comparison routine to avoid a simple early-exit comparison.

---

# File Encryption

Select:

```text
3. Encrypt file
```

The program asks:

```text
Input file: secret.txt
Output encrypted file: secret.dead
Password:
```

The resulting file contains encrypted binary data.

For example:

```text
secret.txt
    |
    v
DEADHASH key derivation
    |
    v
encryption key
    |
    v
stream encryption
    |
    v
secret.dead
```

---

# File Decryption

Select:

```text
4. Decrypt file
```

For example:

```text
Encrypted file: secret.dead
Output decrypted file: recovered.txt
Password:
```

If the correct password is supplied:

```text
secret.dead
    |
    v
read salt + nonce
    |
    v
derive same key
    |
    v
decrypt
    |
    v
recovered.txt
```

---

# Testing

Create a test file:

```bash
echo "This is a secret file." > secret.txt
```

Run DEADHASH:

```bash
./deadhash
```

Select:

```text
3
```

Enter:

```text
Input file: secret.txt
Output encrypted file: secret.dead
Password:
```

Then decrypt it:

```text
4
```

Enter:

```text
Encrypted file: secret.dead
Output decrypted file: recovered.txt
Password:
```

Check the recovered file:

```bash
cat recovered.txt
```

Expected output:

```text
This is a secret file.
```

You can also compare the original and recovered files:

```bash
cmp secret.txt recovered.txt
```

If there is no output, the files are identical.

---

# Educational Encryption Design

The current educational encryption implementation uses a password-derived key and a generated nonce.

The general design is:

```text
                 PASSWORD
                     |
                     v
               Key Derivation
                     |
                     v
                 256-bit key
                     |
                     +
                     |
                  NONCE
                     |
                     v
             Keystream Generator
                     |
                     v
              Encryption Stream
                     |
                     v
                  FILE DATA
```

The encryption operation uses XOR:

```text
plaintext XOR keystream = ciphertext
```

And decryption uses the same operation:

```text
ciphertext XOR keystream = plaintext
```

This works because:

```text
A XOR B XOR B = A
```

However, the security of such a construction depends completely on how the keystream is generated and how keys/nonces are managed.

The current implementation is therefore **educational and not production-grade encryption**.

---

# Encrypted File Format

The educational encrypted file begins with a small header.

Conceptually:

```text
+----------------+
| Magic          |
+----------------+
| Version        |
+----------------+
| Salt           |
+----------------+
| Nonce          |
+----------------+
| Encrypted data |
+----------------+
```

The magic value identifies the file as a DEADHASH encrypted file.

The version allows the format to evolve in the future.

The salt allows the password-derived key to be recreated.

The nonce is used by the encryption stream.

The encrypted data contains the transformed file contents.

---

# Randomness

DEADHASH uses:

```text
/dev/urandom
```

on Linux for random salt and nonce generation.

The project does not use a predictable value such as:

```c
time(NULL)
```

for cryptographic salts or nonces.

Random values are important because predictable salts and nonces can seriously weaken cryptographic systems.

---

# Memory Usage

The hashing implementation allocates a configurable memory region.

Conceptually:

```text
Password
   |
   v
Initial state
   |
   v
+----------------------+
| Memory region        |
|                      |
| Block 0              |
| Block 1              |
| Block 2              |
| ...                  |
| Block N              |
+----------------------+
   |
   v
Memory mixing
   |
   v
Finalization
   |
   v
Hash
```

The purpose is to make password guessing more expensive by requiring both computation and memory.

---

# Password Stretching

The algorithm performs repeated computation controlled by:

```c
DEADHASH_ROUNDS
```

For example:

```c
#define DEADHASH_ROUNDS 100000
```

Increasing the number of rounds generally increases the amount of computation required.

This is useful conceptually for understanding password hashing because attackers may attempt many password guesses.

However, these parameters have **not been calibrated or cryptographically reviewed**, so they should not be treated as secure production settings.

---

# Constant-Time Comparison

A normal comparison might stop as soon as it finds a mismatch:

```text
AAAA
AAAB
  ^
  mismatch
  stop
```

A constant-time comparison processes all bytes:

```text
AAAA
AAAB
^^^^
compare everything
```

DEADHASH uses an XOR-based comparison:

```c
difference |= a[i] ^ b[i];
```

and checks the final result only after processing all bytes.

This helps avoid simple timing differences caused by early exits.

---

# Secure Password Input

On Linux, DEADHASH temporarily disables terminal echo.

Instead of:

```text
Password: MyPassword123
```

the terminal displays:

```text
Password:
```

while the user types.

The terminal settings are restored afterward.

This protects against accidentally displaying the password on the terminal.

It does **not** protect against:

* Keyloggers
* Malware
* Debuggers
* Privileged processes
* Compromised operating systems

---

# Memory Cleanup

The program attempts to clear sensitive values from memory after use.

For example:

```c
memset(
    password,
    0,
    sizeof(password)
);
```

The project also clears temporary key material and internal state.

However, ordinary `memset()` is not guaranteed by the C standard to prevent an optimizing compiler from removing the operation in every situation.

Production cryptographic software should use a vetted secure-memory-clearing primitive.

---

# Security Limitations

The current educational implementation has important limitations.

It has **not** received:

* Formal cryptographic analysis
* Independent security auditing
* Cryptanalysis
* Side-channel analysis
* Production testing
* Cross-platform security review

The custom hashing algorithm should therefore be considered a learning exercise.

The file encryption system also lacks the properties expected from modern authenticated encryption.

For example, the current educational implementation does not provide a cryptographically secure authentication tag.

That means an attacker could potentially modify encrypted data without the program being able to reliably detect the modification.

---

# Production Cryptography

Do not invent cryptographic algorithms for production systems.

Modern applications should generally rely on established constructions and well-maintained cryptographic libraries.

Examples include:

```text
Password hashing:
    Argon2id

Authenticated encryption:
    XChaCha20-Poly1305
    AES-256-GCM
```

A production architecture could look like:

```text
                 PASSWORD
                     |
                     v
                  Argon2id
                     |
                     v
                 256-bit key
                     |
                     v
             XChaCha20-Poly1305
                     |
              +------+------+
              |             |
           nonce         auth tag
              |             |
              +------+------+
                     |
                     v
                encrypted file
```

This provides both:

```text
Confidentiality
+
Integrity / authentication
```

which the current educational XOR-based implementation does not provide.

---

# What This Project Teaches

DEADHASH is intended to demonstrate the concepts behind:

* Password hashing
* Salts
* Key derivation
* Password stretching
* Memory-hard computation
* Randomness
* Nonces
* Symmetric encryption
* XOR
* Streaming encryption
* Binary file formats
* Constant-time comparison
* Secure password input
* Memory cleanup
* Cryptographic design limitations

The main goal is understanding **why cryptographic systems are designed the way they are**, rather than creating a replacement for established cryptographic libraries.

---

# Example Workflow

## Create a password hash

```text
./deadhash

1. Hash password
```

Result:

```text
DEADHASH$1$100000$1024$SALT$HASH
```

---

## Verify the password

```text
./deadhash

2. Verify password
```

Enter the stored record and password.

Result:

```text
PASSWORD CORRECT
```

or:

```text
PASSWORD INCORRECT
```

---

## Encrypt a file

```text
./deadhash

3. Encrypt file
```

Example:

```text
Input file: secret.txt
Output encrypted file: secret.dead
Password:
```

---

## Decrypt a file

```text
./deadhash

4. Decrypt file
```

Example:

```text
Encrypted file: secret.dead
Output decrypted file: recovered.txt
Password:
```

---

# Building

Standard build:

```bash
gcc -Wall -Wextra -std=c11 main.c deadhash.c -o deadhash
```

Run:

```bash
./deadhash
```

---

# Cleaning the Build

Remove the compiled executable:

```bash
rm -f deadhash
```

Then rebuild:

```bash
gcc -Wall -Wextra -std=c11 main.c deadhash.c -o deadhash
```

---

# GitHub

Initialize the repository:

```bash
git init
```

Add the files:

```bash
git add .
```

Create the first commit:

```bash
git commit -m "Initial DEADHASH implementation"
```

Connect your GitHub repository:

```bash
git remote add origin YOUR_REPOSITORY_URL
```

Push:

```bash
git branch -M main
git push -u origin main
```

---

# License

DEADHASH is released under the MIT License.

See:

```text
LICENSE
```

for the complete license text.

---

# Disclaimer

This software is provided for educational purposes.

The author makes no guarantee that the cryptographic algorithms, parameters, implementation, file format, or security properties are suitable for protecting sensitive information.

Do not rely on DEADHASH for production security.

---

# Project Goals

Future educational improvements may include:

* Authenticated encryption
* XChaCha20-Poly1305
* Argon2id
* Secure memory handling
* Better file format versioning
* File integrity verification
* Key separation
* Cryptographic test vectors
* Automated tests
* Fuzz testing
* Error-handling improvements
* Cross-platform support
* Security review

---

# Final Note

DEADHASH is primarily a project for learning.

The most important lesson is not:

> "I created my own encryption algorithm."

The important lesson is understanding why cryptography is difficult, why seemingly strong algorithms can contain subtle weaknesses, and why real security software relies on algorithms that have been publicly analyzed and reviewed.

```

This README is appropriate for the **current educational implementation** and is intentionally honest about the fact that the custom crypto isn't production-safe. If you later replace the custom encryption with Argon2id + XChaCha20-Poly1305, the security sections should be updated accordingly.
```
<img width="275" height="189" alt="Screenshot From 2026-09-01 19-19-50" src="https://github.com/user-attachments/assets/cd18a25a-488c-442a-8317-f85a2c554bc8" />
