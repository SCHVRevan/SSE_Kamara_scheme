#ifndef SSE_TYPES_H
#define SSE_TYPES_H

#include <vector>
#include <string>
#include <array>
#include <tuple>
#include <boost/beast/core/detail/base64.hpp>

using namespace std;
using LambdaTuple = tuple<
    string,
    string,
    tuple<string, string, string>,
    tuple<string, string, string, string, string, string, string>
>;

// F(w), G(w), P(w)
struct SearchToken {
    string t1;
    string t2;
    string t3;
};

struct AddToken {
    string t1;  // F(id)
    string t2;  // G(id)
    //vector<tuple<F(w), G(w), ((id, 0, 0)^H1(P(wi))), ((0, 0, 0, 0, 0, 0, F(wi))^H2(P(id)))>>;
    vector<LambdaTuple> lambdas;
    string encrypted_file;
};

// F(id), G(id), P(id), id
struct DelToken {
    string t1;
    string t2;
    string t3;
};

#endif