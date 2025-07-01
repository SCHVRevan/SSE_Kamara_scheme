#ifndef BASE64_H
#define BASE64_H

#include <vector>
#include <string>
#include <cstdint>
using namespace std;

class Base64 {
public:
    static string encode(const vector<uint8_t>& data);
    static vector<uint8_t> decode(const string& encoded_string);
    static vector<uint8_t> xor_vectors(const vector<uint8_t>& v1, const vector<uint8_t>& v2);
};

#endif
