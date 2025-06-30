#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <iostream>
#include <cstdio>
#include <vector>
#include <string>
#include <random>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <fstream>
#include <filesystem>
#include <openssl/hmac.h>
#include <boost/algorithm/hex.hpp>
#include <sstream>
using namespace std;

// Signelton
class encryption {
    encryption() {};
    ~encryption() {};
    encryption(encryption const&);
    encryption& operator=(encryption const&);
    vector<vector <unsigned char>> key;
public:
    static encryption& Instance();
    vector<unsigned char> pbkdf2(const string &, const vector<unsigned char> &, int, int);
    vector<unsigned char> getSalt(const string&);
    void setKey(const string&);
    string encrypt_files(const string&);
    string decrypt_files(const string&);
    string encrypt(const string&);
    string decrypt(const string&);
    string H1(const string&);
    string H2(const string&);
    string G(const string&);
    string P(const string&);
    string F(const string&);
};

#endif
