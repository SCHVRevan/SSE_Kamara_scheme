#ifndef CIPHER_H
#define CIPHER_H

#include <vector>
#include <fstream>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <stdexcept>
#include <cstring>
using namespace std;

class cipher {
public:
    static void generate_iv(unsigned char*);
    static vector<unsigned char> encrypt_data(const vector<unsigned char>& plaintext, const vector<unsigned char>& key, unsigned char* iv);
    static vector<unsigned char> decrypt_data(const vector<unsigned char>& ciphertext, const vector<unsigned char>& key, const unsigned char* iv);
    static vector<unsigned char> encrypt_file(const string&, const vector<unsigned char>&);
    static vector<unsigned char> decrypt_file(const string&, const vector<unsigned char>&);
};

#endif
