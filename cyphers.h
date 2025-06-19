#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

// AES-256-CBC
int encrypt_aes256_cbc(const char *, const char *, const char *);
int decrypt_aes256_cbc(const char *, const char *, const char *);

// ChaCha20
int encrypt_chacha20(const char *infile, const char *outfile, const char *password);
int decrypt_chacha20(const char *infile, const char *outfile, const char *password);

void run_symmetric(const int alg, const int action, const char *in, const char *out, const char *key);

#endif