#include "cipher.h"
#include <stdexcept>

void cipher::generate_iv(unsigned char* iv) {
    if (1 != RAND_bytes(iv, EVP_MAX_IV_LENGTH)) {
        throw runtime_error("RAND_bytes failed for IV generation");
    }
}

vector<unsigned char> cipher::encrypt_data(const vector<unsigned char>& plaintext, const vector<unsigned char>& key, unsigned char* iv) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw runtime_error("EVP_CIPHER_CTX_new failed");

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key.data(), iv)) {
        EVP_CIPHER_CTX_free(ctx);
        throw runtime_error("EVP_EncryptInit_ex failed");
    }

    vector<unsigned char> ciphertext(plaintext.size() + EVP_MAX_BLOCK_LENGTH);
    int len = 0, ciphertext_len = 0;

    if (1 != EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), plaintext.size())) {
        EVP_CIPHER_CTX_free(ctx);
        throw runtime_error("EVP_EncryptUpdate failed");
    }
    ciphertext_len = len;

    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        throw runtime_error("EVP_EncryptFinal_ex failed");
    }
    ciphertext_len += len;
    ciphertext.resize(ciphertext_len);

    EVP_CIPHER_CTX_free(ctx);
    return ciphertext;
}

vector<unsigned char> cipher::decrypt_data(const vector<unsigned char>& ciphertext, const vector<unsigned char>& key, const unsigned char* iv) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw runtime_error("EVP_CIPHER_CTX_new failed");

    const EVP_CIPHER* cipher = EVP_aes_256_cbc();
    int block_size = EVP_CIPHER_block_size(cipher);
    
    if (ciphertext.size() % block_size != 0) {
        EVP_CIPHER_CTX_free(ctx);
        throw runtime_error("Ciphertext size must be multiple of block size");
    }

    if (1 != EVP_DecryptInit_ex(ctx, cipher, NULL, key.data(), iv)) {
        EVP_CIPHER_CTX_free(ctx);
        throw runtime_error("EVP_DecryptInit_ex failed");
    }

    vector<unsigned char> plaintext(ciphertext.size() + EVP_MAX_BLOCK_LENGTH);
    int len = 0, plaintext_len = 0;

    if (1 != EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), ciphertext.size())) {
        EVP_CIPHER_CTX_free(ctx);
        throw runtime_error("EVP_DecryptUpdate failed");
    }
    plaintext_len = len;

    if (1 != EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        throw runtime_error("EVP_DecryptFinal_ex failed");
    }
    plaintext_len += len;
    plaintext.resize(plaintext_len);

    EVP_CIPHER_CTX_free(ctx);
    return plaintext;
}

vector<unsigned char> cipher::encrypt_file(const string& file_path, const vector<unsigned char>& key) {
    ifstream in(file_path, ios::binary);
    if (!in) return {};

    vector<unsigned char> plaintext((istreambuf_iterator<char>(in)), {});
    unsigned char iv[EVP_MAX_IV_LENGTH];
    generate_iv(iv);

    auto ciphertext = encrypt_data(plaintext, key, iv);

    vector<unsigned char> result;
    result.insert(result.end(), iv, iv + EVP_MAX_IV_LENGTH);
    result.insert(result.end(), ciphertext.begin(), ciphertext.end());

    return result;
}

vector<unsigned char> cipher::decrypt_file(const string& file_path, const vector<unsigned char>& key) {
    ifstream in(file_path, ios::binary);
    if (!in) return {};

    unsigned char iv[EVP_MAX_IV_LENGTH];
    in.read(reinterpret_cast<char*>(iv), EVP_MAX_IV_LENGTH);

    vector<unsigned char> ciphertext((istreambuf_iterator<char>(in)), {});

    try {
        return decrypt_data(ciphertext, key, iv);
    } catch (...) {
        return {};
    }
}
