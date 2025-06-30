#include "token_gen.h"

void token_generator::gen_ta(const string& file_path, AddToken& token) {
    encryption& e = encryption::Instance();

    string file_id = word_getter::extract_filename(file_path);
    if (file_id == "") {
        throw runtime_error("File not found: " + file_path);
    }
    token.t1 = e.F(file_id);
    token.t2 = e.G(file_id);
    
    unordered_set<string> words;
    string h1_pw = "";
    string h2_pf = "";
    word_getter::process_file(file_path, words);
    for (const auto& w: words) {
        h1_pw = e.H1(e.P(w));
        h2_pf = e.H2(e.P(file_id));
        
        tuple<string, string, string> tuple1 = {
            base64::xor_strings(file_id, h1_pw),
            h1_pw, h1_pw
        };
        
        tuple<string, string, string, string, string, string, string> tuple2 = {
            h2_pf, h2_pf, h2_pf, h2_pf, h2_pf, h2_pf,
            base64::xor_strings(e.F(w), h2_pf)
        };
        
        token.lambdas.emplace_back(e.F(w), e.G(w), tuple1, tuple2);
    }

    token.encrypted_file = base64::encode(e.encrypt_files(file_path));
}

void token_generator::gen_td(const string& file_path, DelToken& token) {
    encryption& e = encryption::Instance();
    string file_id = word_getter::extract_filename(file_path);
    if (file_id == "") {
        throw runtime_error("File not found: " + file_path);
    }
    token.t1 = e.F(file_id);
    token.t2 = e.G(file_id);
    token.t2 = e.P(file_id);
}

void token_generator::gen_ts(const string& word, SearchToken& token) {
    encryption& e = encryption::Instance();
    token.t1 = e.F(word);
    token.t2 = e.G(word);
    token.t3 = e.P(word);
}