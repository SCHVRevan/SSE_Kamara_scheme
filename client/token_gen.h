#ifndef TOKEN_GEN_H
#define TOKEN_GEN_H

#include <vector>
#include <string>
#include <unordered_set>
#include <tuple>
#include "../common/sse_types.h"
#include "encryption.h"
#include "../common/base64.h"

using namespace std;

class token_generator {
public:
    static unordered_set<string> process_file(const string& file_content);
    static void gen_ta(const string& file_id, const string& file_content, AddToken& token);
    static void gen_td(const string& file_id, DelToken& token);
    static void gen_ts(const string& word, SearchToken& token);
};

#endif
