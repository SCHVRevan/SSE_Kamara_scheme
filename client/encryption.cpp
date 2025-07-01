#include "encryption.h"
#include "cipher.h"

encryption& encryption::Instance() {
    static encryption e;
    return e;
}

vector<unsigned char> encryption::pbkdf2(const string &password, const vector<unsigned char> &salt, int iterations, int keyLength) {
    vector<unsigned char> key(keyLength);
    
    if (PKCS5_PBKDF2_HMAC(password.c_str(), password.length(), salt.data(), salt.size(), iterations, EVP_sha256(), keyLength, key.data()) == 0) {
        throw runtime_error("PBKDF2 key derivation failed");
    }
    
    return key;
}

vector<unsigned char> encryption::getSalt(const string& input) {
    vector<unsigned char> res(16);

    int salt_gen_key = 0;
    for (char c: input) salt_gen_key += static_cast<int>(c);

    mt19937_64 generator(salt_gen_key);
    for (int i = 0; i < res.size(); i++) {
        res[i] = static_cast<unsigned char>(generator() % 256);
    }

    return res;
}

void encryption::setKey(const string& new_key) {  
    vector<unsigned char> salt = getSalt(new_key);
    vector<vector<unsigned char>> tmp_key(4);
    for (int i = 0; i < 4; i++) {
        string salt_str(salt.begin(), salt.end());
        salt = getSalt(salt_str);
        tmp_key[i] = pbkdf2(new_key, salt, 100000, 32);
    }
    
    key = tmp_key;
}

vector<unsigned char> encryption::encrypt_files(const string& path) {
    try {
        if (filesystem::is_regular_file(path)) {
            return cipher::encrypt_file(path, key[3]);
        }
    } catch (const filesystem::filesystem_error& e) {
        throw runtime_error(string("Error processing file: ") + e.what());
    } catch (const exception& e) {
        throw runtime_error(string("Encryption error: ") + e.what());
    }
    return {};
}

vector<unsigned char> encryption::encrypt_files(const vector<unsigned char>& data) {
    try {
        unsigned char iv[EVP_MAX_IV_LENGTH];
        cipher::generate_iv(iv);
        auto ciphertext = cipher::encrypt_data(data, key[3], iv);
        
        vector<unsigned char> result;
        result.insert(result.end(), iv, iv + EVP_MAX_IV_LENGTH);
        result.insert(result.end(), ciphertext.begin(), ciphertext.end());
        return result;
    } catch (const exception& e) {
        throw runtime_error(string("Encryption error: ") + e.what());
    }
}

vector<unsigned char> encryption::decrypt_files(const vector<unsigned char>& data) {
    try {
        if (data.size() < EVP_MAX_IV_LENGTH) {
            throw runtime_error("Invalid encrypted data size");
        }
        
        unsigned char iv[EVP_MAX_IV_LENGTH];
        copy(data.begin(), data.begin() + EVP_MAX_IV_LENGTH, iv);
        
        vector<unsigned char> ciphertext(data.begin() + EVP_MAX_IV_LENGTH, data.end());
        return cipher::decrypt_data(ciphertext, key[3], iv);
    } catch (const exception& e) {
        throw runtime_error(string("Decryption error: ") + e.what());
    }
}

vector<unsigned char> encryption::decrypt_files(const string& path) {
    try {
        if (filesystem::is_regular_file(path)) {
            return cipher::decrypt_file(path, key[3]);
        }
    } catch (const filesystem::filesystem_error& e) {
        throw runtime_error(string("Error processing file: ") + e.what());
    } catch (const exception& e) {
        throw runtime_error(string("Decryption error: ") + e.what());
    }
    return {};
}

vector<unsigned char> encryption::H1(const vector<unsigned char>& input) {
    const EVP_MD* md = EVP_sha256();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    
    EVP_Digest(input.data(), input.size(), hash, &hash_len, md, NULL);
    assert(hash_len == 32);
    return vector<unsigned char>(hash, hash + hash_len);
}

vector<unsigned char> encryption::H2(const vector<unsigned char>& input) {
    return H1(input);
}

vector<unsigned char> encryption::F(const string& word) {
    unsigned char hmac[SHA256_DIGEST_LENGTH];
    unsigned int hmac_len;
    
    HMAC(EVP_sha256(), key[0].data(), key[0].size(), reinterpret_cast<const unsigned char*>(word.c_str()), word.length(), hmac, &hmac_len);
 
    return vector<unsigned char>(hmac, hmac + hmac_len);
}

vector<unsigned char> encryption::G(const string& word) {
    unsigned char hmac[SHA256_DIGEST_LENGTH];
    unsigned int hmac_len;
    
    HMAC(EVP_sha256(), key[1].data(), key[1].size(), reinterpret_cast<const unsigned char*>(word.c_str()), word.length(), hmac, &hmac_len);
    
    return vector<unsigned char>(hmac, hmac + hmac_len);
}

vector<unsigned char> encryption::P(const string& word) {
    unsigned char hmac[SHA256_DIGEST_LENGTH];
    unsigned int hmac_len;
    
    HMAC(EVP_sha256(), key[2].data(), key[2].size(), reinterpret_cast<const unsigned char*>(word.c_str()), word.length(), hmac, &hmac_len);
    
    return vector<unsigned char>(hmac, hmac + hmac_len);
}
