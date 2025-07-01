#ifndef SSE_TYPES_H
#define SSE_TYPES_H

#include <vector>
#include <string>
#include <array>
#include <tuple>
#include <boost/beast/core/detail/base64.hpp>

using namespace std;
using LambdaTuple = tuple<
    vector<uint8_t>,
    vector<uint8_t>,
    tuple<vector<uint8_t>, vector<uint8_t>, vector<uint8_t>>,
    tuple<vector<uint8_t>, vector<uint8_t>, vector<uint8_t>, vector<uint8_t>, vector<uint8_t>, vector<uint8_t>, vector<uint8_t>>
>;

// F(w), G(w), P(w)
struct SearchToken {
    vector<uint8_t> t1;
    vector<uint8_t> t2;
    vector<uint8_t> t3;
};

struct AddToken {
    vector<uint8_t> t1;  // F(id)
    vector<uint8_t> t2;  // G(id)
    //vector<tuple<F(w), G(w), ((id, 0, 0)^H1(P(wi))), ((0, 0, 0, 0, 0, 0, F(wi))^H2(P(id)))>>;
    vector<LambdaTuple> lambdas;
};

// F(id), G(id), P(id), id
struct DelToken {
    vector<uint8_t> t1;
    vector<uint8_t> t2;
    vector<uint8_t> t3;
};

#endif
