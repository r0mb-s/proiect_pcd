void run_symmetric(const char alg, const charaction, const char in, const charout, const char *pass)
{
    if (strcasecmp(alg, "AES") == 0)
    {
        if (strcasecmp(action, "encrypt") == 0)
        {
            if (!encrypt_aes256_cbc(in, out, pass))
                fprintf(stderr, "AES encryption failed.\n");
            else
                printf("AES-256-CBC encryption succeeded.\n");
        }
        else if (strcasecmp(action, "decrypt") == 0)
        {
            if (!decrypt_aes256_cbc(in, out, pass))
                fprintf(stderr, "AES decryption failed.\n");
            else
                printf("AES-256-CBC decryption succeeded.\n");
        }
        else
        {
            fprintf(stderr, "Unknown action: %s\n", action);
        }
    }
    else if (strcasecmp(alg, "CHACHA") == 0)
    {
        if (strcasecmp(action, "encrypt") == 0)
        {
            if (!encrypt_chacha20(in, out, pass))
                fprintf(stderr, "ChaCha20 encryption failed.\n");
            else
                printf("ChaCha20 encryption succeeded.\n");
        }
        else if (strcasecmp(action, "decrypt") == 0)
        {
            if (!decrypt_chacha20(in, out, pass))
                fprintf(stderr, "ChaCha20 decryption failed.\n");
            else
                printf("ChaCha20 decryption succeeded.\n");
        }
        else
        {
            fprintf(stderr, "Unknown action: %s\n", action);
        }
    }
    else
    {
        fprintf(stderr, "Unknown algorithm: %s\n", alg);
    }
}