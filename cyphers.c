#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <string.h>
#include <stdio.h>

#define SALT_SIZE 8
#define KEY_SIZE 32
#define IV_SIZE 16
#define NONCE_SIZE 12
#define BUF_SIZE 4096

void handle_errors(const char *msg)
{
    fprintf(stderr, "[ERROR] %s\n", msg);
    ERR_print_errors_fp(stderr);
}

int encrypt_aes256_cbc(const char *infile, const char *outfile, const char *password)
{
    FILE *fin = fopen(infile, "rb");
    FILE *fout = fopen(outfile, "wb");
    if (!fin || !fout)
    {
        perror("fopen");
        return 0;
    }

    if (fseek(fin, 20, SEEK_SET) != 0)
    {
        perror("fseek");
        fclose(fin);
        fclose(fout);
        return 0;
    }

    // unsigned char buffer[BUF_SIZE];
    // size_t bytesRead;
    // while ((bytesRead = fread(buffer, 1, BUF_SIZE, fin)) > 0)
    // {
    //     fwrite(buffer, 1, bytesRead, stdout); // Print to console
    // }
    // fwrite("\n", 1, 1, stdout);

    // return 1;

    unsigned char key[KEY_SIZE], iv[IV_SIZE];
    if (!EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha256(), NULL,
                        (unsigned char *)password, strlen(password), 10000, key, iv))
    {
        handle_errors("EVP_BytesToKey failed");
        return 0;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        handle_errors("EVP_CIPHER_CTX_new failed");
        return 0;
    }

    if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
    {
        handle_errors("EncryptInit failed");
        return 0;
    }

    unsigned char inbuf[BUF_SIZE], outbuf[BUF_SIZE + EVP_MAX_BLOCK_LENGTH];
    int inlen, outlen;

    while ((inlen = fread(inbuf, 1, BUF_SIZE, fin)) > 0)
    {
        if (!EVP_EncryptUpdate(ctx, outbuf, &outlen, inbuf, inlen))
        {
            handle_errors("EncryptUpdate failed");
            return 0;
        }
        fwrite(outbuf, 1, outlen, fout);
    }

    if (!EVP_EncryptFinal_ex(ctx, outbuf, &outlen))
    {
        handle_errors("EncryptFinal failed");
        return 0;
    }
    fwrite(outbuf, 1, outlen, fout);

    EVP_CIPHER_CTX_free(ctx);
    fclose(fin);
    fclose(fout);
    return 1;
}

int decrypt_aes256_cbc(const char *infile, const char *outfile, const char *password)
{
    FILE *fin = fopen(infile, "rb");
    FILE *fout = fopen(outfile, "wb");
    if (!fin || !fout)
    {
        perror("fopen");
        return 0;
    }

    if (fseek(fin, 20, SEEK_SET) != 0)
    {
        perror("fseek");
        fclose(fin);
        fclose(fout);
        return 0;
    }

    unsigned char key[KEY_SIZE], iv[IV_SIZE];
    if (!EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha256(), NULL,
                        (unsigned char *)password, strlen(password), 10000, key, iv))
    {
        handle_errors("EVP_BytesToKey failed");
        return 0;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        handle_errors("EVP_CIPHER_CTX_new failed");

    if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handle_errors("DecryptInit failed");

    unsigned char inbuf[BUF_SIZE], outbuf[BUF_SIZE + EVP_MAX_BLOCK_LENGTH];
    int inlen, outlen;

    while ((inlen = fread(inbuf, 1, BUF_SIZE, fin)) > 0)
    {
        if (!EVP_DecryptUpdate(ctx, outbuf, &outlen, inbuf, inlen))
        {
            handle_errors("DecryptUpdate failed");
            return 0;
        }
        fwrite(outbuf, 1, outlen, fout);
    }

    if (!EVP_DecryptFinal_ex(ctx, outbuf, &outlen))
    {
        handle_errors("DecryptFinal failed");
        return 0;
    }
    fwrite(outbuf, 1, outlen, fout);

    EVP_CIPHER_CTX_free(ctx);
    fclose(fin);
    fclose(fout);
    return 1;
}

int encrypt_chacha20(const char *infile, const char *outfile, const char *password)
{
    FILE *fin = fopen(infile, "rb");
    FILE *fout = fopen(outfile, "wb");
    if (!fin || !fout)
    {
        perror("fopen");
        return 0;
    }

    unsigned char salt[SALT_SIZE];
    unsigned char nonce[NONCE_SIZE];
    if (!RAND_bytes(salt, SALT_SIZE) || !RAND_bytes(nonce, NONCE_SIZE))
    {
        handle_errors("Salt/Nonce generation failed");
        return 0;
    }

    unsigned char key[KEY_SIZE];
    if (!EVP_BytesToKey(EVP_chacha20(), EVP_sha256(), salt,
                        (unsigned char *)password, strlen(password), 10000, key, NULL))
    {
        handle_errors("EVP_BytesToKey failed for ChaCha20");
        return 0;
    }

    fwrite("Salted__", 1, 8, fout);
    fwrite(salt, 1, SALT_SIZE, fout);
    fwrite(nonce, 1, NONCE_SIZE, fout);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        handle_errors("EVP_CIPHER_CTX_new failed");

    if (!EVP_EncryptInit_ex(ctx, EVP_chacha20(), NULL, key, nonce))
        handle_errors("EncryptInit failed");

    unsigned char inbuf[BUF_SIZE], outbuf[BUF_SIZE];
    int inlen, outlen;

    while ((inlen = fread(inbuf, 1, BUF_SIZE, fin)) > 0)
    {
        if (!EVP_EncryptUpdate(ctx, outbuf, &outlen, inbuf, inlen))
            handle_errors("EncryptUpdate failed");
        fwrite(outbuf, 1, outlen, fout);
    }

    EVP_CIPHER_CTX_free(ctx);
    fclose(fin);
    fclose(fout);
    return 1;
}

int decrypt_chacha20(const char *infile, const char *outfile, const char *password)
{
    FILE *fin = fopen(infile, "rb");
    FILE *fout = fopen(outfile, "wb");
    if (!fin || !fout)
    {
        perror("fopen");
        return 0;
    }

    unsigned char header[8];
    if (fread(header, 1, 8, fin) != 8 || memcmp(header, "Salted__", 8) != 0)
    {
        fprintf(stderr, "Invalid header (no Salted__)\n");
        return 0;
    }

    unsigned char salt[SALT_SIZE], nonce[NONCE_SIZE];
    fread(salt, 1, SALT_SIZE, fin);
    fread(nonce, 1, NONCE_SIZE, fin);

    unsigned char key[KEY_SIZE];
    if (!EVP_BytesToKey(EVP_chacha20(), EVP_sha256(), salt,
                        (unsigned char *)password, strlen(password), 10000, key, NULL))
    {
        handle_errors("Key derivation failed");
        return 0;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        handle_errors("EVP_CIPHER_CTX_new failed");

    if (!EVP_DecryptInit_ex(ctx, EVP_chacha20(), NULL, key, nonce))
        handle_errors("DecryptInit failed");

    unsigned char inbuf[BUF_SIZE], outbuf[BUF_SIZE];
    int inlen, outlen;

    while ((inlen = fread(inbuf, 1, BUF_SIZE, fin)) > 0)
    {
        if (!EVP_DecryptUpdate(ctx, outbuf, &outlen, inbuf, inlen))
            handle_errors("DecryptUpdate failed");
        fwrite(outbuf, 1, outlen, fout);
    }

    EVP_CIPHER_CTX_free(ctx);
    fclose(fin);
    fclose(fout);
    return 1;
}

void run_symmetric(const int alg, const int action, const char *in, const char *out, const char *key)
{
    if (alg  == 0)
    {
        if (action == 0)
        {
            if (!encrypt_aes256_cbc(in, out, key))
                fprintf(stderr, "AES encryption failed.\n");
            else
                printf("AES-256-CBC encryption succeeded.\n");
        }
        else if (action == 1)
        {
            if (!decrypt_aes256_cbc(in, out, key))
                fprintf(stderr, "AES decryption failed.\n");
            else
                printf("AES-256-CBC decryption succeeded.\n");
        }
        else
        {
            fprintf(stderr, "Unknown action: %d\n", action);
        }
    }
    else if (alg  == 1)
    {
        if (action == 0)
        {
            if (!encrypt_chacha20(in, out, key))
                fprintf(stderr, "ChaCha20 encryption failed.\n");
            else
                printf("ChaCha20 encryption succeeded.\n");
        }
        else if (action == 1)
        {
            if (!decrypt_chacha20(in, out, key))
                fprintf(stderr, "ChaCha20 decryption failed.\n");
            else
                printf("ChaCha20 decryption succeeded.\n");
        }
        else
        {
            fprintf(stderr, "Unknown action: %d\n", action);
        }
    }
    else
    {
        fprintf(stderr, "Unknown algorithm: %d\n", alg);
    }
}