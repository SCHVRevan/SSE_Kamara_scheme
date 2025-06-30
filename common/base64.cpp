#include "base64.h"

string base64::encode(const string& data) {
    string encoded;
    encoded.resize(boost::beast::detail::base64::encoded_size(data.size()));
    boost::beast::detail::base64::encode(encoded.data(), data.data(), data.size());
    return encoded;
}

string base64::decode(const string& data) {
    string decoded;
    decoded.resize(boost::beast::detail::base64::decoded_size(data.size()));
    auto result = boost::beast::detail::base64::decode(decoded.data(), data.data(), data.size());
    decoded.resize(result.first);
    return decoded;
}

string base64::xor_strings(const string& a, const string& b) {
    string result;
    const size_t max_len = max(a.size(), b.size());
    
    for (size_t i = 0; i < max_len; ++i) {
        // Если символ выходит за границы строки, берём '\0' (нулевой байт)
        char a_char = (i < a.size()) ? a[i] : '\0';
        char b_char = (i < b.size()) ? b[i] : '\0';
        
        result.push_back(a_char ^ b_char);
    }
    
    // Обрезаем нулевые байты в начале строки
    size_t first_non_zero = 0;
    while (first_non_zero < result.size() && result[first_non_zero] == '\0') {
        first_non_zero++;
    }
    
    if (first_non_zero > 0) {
        result = result.substr(first_non_zero);
    }
    
    return result;
}

json base64::serialize_lambda(const auto& lambda) {
    const auto& f_w = get<0>(lambda);   // F(w)
    const auto& g_w = get<1>(lambda);   // G(w)
    const auto& tuple1 = get<2>(lambda);    // (id, 0, 0) ^ H1(P(wi))
    const auto& tuple2 = get<3>(lambda);    // (0, ..., F(wi)) ^ H2(P(id))

    return {
        f_w,
        g_w,
        {
            get<0>(tuple1),
            get<1>(tuple1),
            get<2>(tuple1)
        },
        {
            get<0>(tuple2),
            get<1>(tuple2),
            get<2>(tuple2),
            get<3>(tuple2),
            get<4>(tuple2),
            get<5>(tuple2),
            get<6>(tuple2)
        }
    };
}

json base64::serialize(const AddToken& token) {
    json request;
    request["action"] = "add";
    request["t1"] = token.t1;
    request["t2"] = token.t2;
    
    // Сериализуем массив lambdas
    json lambdas_array = json::array();
    for (const auto& lambda : token.lambdas) {
        lambdas_array.push_back(serialize_lambda(lambda));
    }
    request["lambdas"] = lambdas_array;
    
    return request;
}

json base64::serialize(const SearchToken& token) {
    return {
        {"action", "search"},
        {"t1", token.t1},
        {"t2", token.t2},
        {"t3", token.t3}
    };
}

json base64::serialize(const DelToken& token) {
    return {
        {"action", "delete"},
        {"t1", token.t1},
        {"t2", token.t2},
        {"t3", token.t3}
    };
}

LambdaTuple base64::deserialize_lambda(const json& j) {
    return {
        j[0].get<string>(), // F(w)
        j[1].get<string>(), // G(w)
        { // tuple1
            j[2][0].get<string>(),
            j[2][1].get<string>(),
            j[2][2].get<string>()
        },
        { // tuple2
            j[3][0].get<string>(),
            j[3][1].get<string>(),
            j[3][2].get<string>(),
            j[3][3].get<string>(),
            j[3][4].get<string>(),
            j[3][5].get<string>(),
            j[3][6].get<string>()
        }
    };
}

AddToken base64::deserialize_add_token(const json& j) {
    AddToken token;
    token.t1 = j["t1"].get<string>();
    token.t2 = j["t2"].get<string>();
    
    for (const auto& item : j["lambdas"]) {
        token.lambdas.push_back(base64::deserialize_lambda(item));
    }
    
    return token;
}

SearchToken base64::deserialize_search_token(const json& j) {
    return {
        j["t1"].get<string>(),
        j["t2"].get<string>(),
        j["t3"].get<string>()
    };
}

DelToken base64::deserialize_del_token(const json& j) {
    return {
        j["t1"].get<string>(),
        j["t2"].get<string>(),
        j["t3"].get<string>()
    };
}
