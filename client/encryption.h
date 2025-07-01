#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <vector>
#include <string>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <filesystem>
#include <random>
#include <fstream>
#include <boost/algorithm/hex.hpp>
using namespace std;

class encryption {
    encryption() {};
    ~encryption() {};
    encryption(encryption const&);
    encryption& operator=(encryption const&);
    vector<vector<unsigned char>> key;
    
public:
    static encryption& Instance();
    vector<unsigned char> pbkdf2(const string& password, const vector<unsigned char>& salt, int iterations, int keyLength);
    vector<unsigned char> getSalt(const string& input);
    void setKey(const string& new_key);
    vector<unsigned char> encrypt_files(const string&);
    vector<unsigned char> decrypt_files(const string&);
    vector<unsigned char> encrypt_files(const vector<unsigned char>&);
    vector<unsigned char> decrypt_files(const vector<unsigned char>&);
    vector<unsigned char> H1(const vector<unsigned char>& input);
    vector<unsigned char> H2(const vector<unsigned char>& input);
    vector<unsigned char> F(const string& word);
    vector<unsigned char> G(const string& word);
    vector<unsigned char> P(const string& word);
};

#endif
