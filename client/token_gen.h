#ifndef TOKEN_GEN_H
#define TOKEN_GEN_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include "encryption.h"
#include "unique_words.h"
#include "../common/sse_types.h"
#include "../common/base64.h"

using namespace std;


class token_generator {
public:
    static void gen_ta(const string& file_path, AddToken&);
    static void gen_td(const string& file_path, DelToken&);
    static void gen_ts(const string& word, SearchToken&);
};

#endif