#include "encryption.h"
#include "cipher.h"

encryption& encryption::Instance() {
    static encryption e;
    return e;
}

vector<unsigned char> encryption::pbkdf2(const string &password, const vector<unsigned char> &salt, int iterations, int keyLength) {
    vector<unsigned char> key(keyLength);
    
    // Calculate key using PKCS5_pbkdf2_hmac
    if (PKCS5_PBKDF2_HMAC(password.c_str(), password.length(), salt.data(), salt.size(), iterations, EVP_sha256(), keyLength, key.data()) == 0) {
        throw runtime_error("PBKDF2 key derivation failed\n");
    }
    
    return key;
}

vector<unsigned char> encryption::getSalt(const string& input) {
    vector<unsigned char> res(16);

    int salt_gen_key = 0;
    for (char c: input) salt_gen_key += static_cast<int>(c);

    mt19937_64 generator(salt_gen_key); // Модифицированный генератор Mersenne Twister
    for (int i = 0; i < res.size(); i++) {
        res[i] = static_cast<unsigned char>(generator() % 256); // Генерация числа от 0 до 255
    }

    return res;
}

void encryption::setKey(const string& new_key) {  
    vector<unsigned char> salt = getSalt(new_key); // 128-bit salt
    vector<vector <unsigned char>> tmp_key(4);
    for (int i = 0; i < 4; i++) {
        string salt_str(salt.begin(), salt.end());
        salt = getSalt(salt_str);
        tmp_key[i] = pbkdf2(new_key, salt, 100000, 32); // key lenght = 32 bytes (256-bit)
    }
    
    key = tmp_key;
}

string encryption::encrypt_files(const string& path) {
    try {
        if (filesystem::is_regular_file(path)) {
            string encrypted = cipher::encrypt_file(path, key[3]);
            if (encrypted.empty()) {
                cout << path << " encryption failed\n";
            } else {
                // Сохраняем зашифрованные данные обратно в файл
                // ofstream out(path, ios::binary);
                // out.write(encrypted.data(), encrypted.size());
                return encrypted;
            }
        }
    } catch (const filesystem::filesystem_error& e) {
        cout << "Error processing file: " << e.what() << '\n';
    } catch (const exception& e) {
        cout << "Encryption error: " << e.what() << '\n';
    }
    return "";
}

string encryption::decrypt_files(const string& path) {
    try {
        if (filesystem::is_regular_file(path)) {
            string decrypted = cipher::decrypt_file(path, key[3]);
            if (decrypted.empty()) {
                cout << path << " wasn't encrypted with AES256 or decryption failed\n";
            } else {
                // Сохраняем расшифрованные данные обратно в файл
                // ofstream out(path, ios::binary);
                // out.write(decrypted.data(), decrypted.size());
                return decrypted;
            }
        }
    } catch (const filesystem::filesystem_error& e) {
        cout << "Error processing file: " << e.what() << '\n';
    } catch (const exception& e) {
        cout << "Decryption error: " << e.what() << '\n';
    }
    return "";
}

string encryption::encrypt(const string& content) {
    return cipher::encrypt_string(content, key[3]);
}

string encryption::decrypt(const string& content) {
    return cipher::decrypt_string(content, key[3]);
}

string encryption::H1(const string& input) {
    const EVP_MD* md = EVP_sha256();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    
    EVP_Digest(input.c_str(), input.length(), hash, &hash_len, md, NULL);
    
    string result;
    boost::algorithm::hex(hash, hash + hash_len, back_inserter(result));
    return result;
}

string encryption::H2(const string& input) {
    return H1(input);
}

string encryption::F(const string& word) {
    unsigned char hmac[SHA256_DIGEST_LENGTH];
    unsigned int hmac_len;
    
    HMAC(EVP_sha256(), key[0].data(), key[0].size(), reinterpret_cast<const unsigned char*>(word.c_str()), word.length(), hmac, &hmac_len);
    
    stringstream ss;
    for (unsigned int i = 0; i < hmac_len; ++i) {
        ss << hex << setw(2) << setfill('0') << static_cast<int>(hmac[i]);
    }
    return ss.str();
}

string encryption::G(const string& word) {
    unsigned char hmac[SHA256_DIGEST_LENGTH];
    unsigned int hmac_len;
    
    HMAC(EVP_sha256(), key[1].data(), key[1].size(), reinterpret_cast<const unsigned char*>(word.c_str()), word.length(), hmac, &hmac_len);
    
    stringstream ss;
    for (unsigned int i = 0; i < hmac_len; ++i) {
        ss << hex << setw(2) << setfill('0') << static_cast<int>(hmac[i]);
    }
    return ss.str();
}

string encryption::P(const string& word) {
    unsigned char hmac[SHA256_DIGEST_LENGTH];
    unsigned int hmac_len;
    
    HMAC(EVP_sha256(), key[2].data(), key[2].size(), reinterpret_cast<const unsigned char*>(word.c_str()), word.length(), hmac, &hmac_len);
    
    stringstream ss;
    for (unsigned int i = 0; i < hmac_len; ++i) {
        ss << hex << setw(2) << setfill('0') << static_cast<int>(hmac[i]);
    }
    return ss.str();
}