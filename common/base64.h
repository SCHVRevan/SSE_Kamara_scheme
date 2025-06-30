#ifndef BASE64_H
#define BASE64_H

#include <string>
#include <vector>
#include <boost/beast/core/detail/base64.hpp>
#include <nlohmann/json.hpp>
#include "sse_types.h"

using namespace std;
using json = nlohmann::json;

class base64 {
public:
    static string encode(const string& data);
    static string decode(const string& data);
    static string xor_strings(const string&, const string&);
    static json serialize_lambda(const auto& lambda);
    static json serialize(const AddToken&);
    static json serialize(const DelToken&);
    static json serialize(const SearchToken&);
    static LambdaTuple deserialize_lambda(const json& lambdas_array);
    static AddToken deserialize_add_token(const json& j);
    static SearchToken deserialize_search_token(const json& j);
    static DelToken deserialize_del_token(const json& j);
};

#endif