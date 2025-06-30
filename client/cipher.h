#ifndef CIPHER_H
#define CIPHER_H

#include <string>
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
        static vector<unsigned char> encrypt_data(const vector<unsigned char>&, const vector<unsigned char>&, unsigned char*);
        static vector<unsigned char> decrypt_data(const vector<unsigned char>&, const vector<unsigned char>&, const unsigned char*);
        static string encrypt_file(const string& , const vector<unsigned char>&);
        static string encrypt_string(const string&, const vector<unsigned char>&);
        static string decrypt_file(const string&, const vector<unsigned char>&);
        static string decrypt_string(const string&, const vector<unsigned char>&);
};

#endif
