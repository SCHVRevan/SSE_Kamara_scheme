#include "token_gen.h"

using namespace std;

unordered_set<string> token_generator::process_file(const string& file_content) {
    unordered_set<string> words;
    string current_word;
        
    for (char c : file_content) {
        if (isalpha(c)) {
            current_word += c;
        } else {
            if (!current_word.empty()) {
                words.insert(current_word);
                current_word.clear();
            }
        }
    }
        
    if (!current_word.empty()) {
        words.insert(current_word);
    }
        
    return words;
}

void token_generator::gen_ta(const string& file_id, const string& file_content, AddToken& token) {
    encryption& e = encryption::Instance();

    token.t1 = e.F(file_id);
    token.t2 = e.G(file_id);
        
    unordered_set<string> words = process_file(file_content);
        
    for (const auto& w : words) {
        vector<uint8_t> f_w = e.F(w);
        vector<uint8_t> g_w = e.G(w);
        vector<uint8_t> p_w = e.P(w);
        vector<uint8_t> p_id = e.P(file_id);
            
        vector<uint8_t> h1_pw = e.H1(p_w);
        vector<uint8_t> h2_pf = e.H2(p_id);
            
        // (id ⊕ H1(P(w)), H1(P(w)), H1(P(w)))
        vector<uint8_t> file_id_data = vector<uint8_t>(file_id.begin(), file_id.end());
        vector<uint8_t> xor_fid_h1 = Base64::xor_vectors(file_id_data, h1_pw);
        tuple<vector<uint8_t>, vector<uint8_t>, vector<uint8_t>> tuple1 = {
            xor_fid_h1,
            h1_pw,
            h1_pw
        };
            
        // (H2(P(id)), ..., F(w) ⊕ H2(P(id)))
        vector<uint8_t> xor_fw_h2 = Base64::xor_vectors(f_w, h2_pf);
        
        tuple<vector<uint8_t>, vector<uint8_t>, vector<uint8_t>, 
            vector<uint8_t>, vector<uint8_t>, vector<uint8_t>,
            vector<uint8_t>> tuple2 = {
                h2_pf, h2_pf, h2_pf, h2_pf, h2_pf, h2_pf,
                xor_fw_h2
            };
        
        token.lambdas.emplace_back(f_w, g_w, tuple1, tuple2);
    }
}

void token_generator::gen_td(const string& file_id, DelToken& token) {
    encryption& e = encryption::Instance();
    token.t1 = e.F(file_id);
    token.t2 = e.G(file_id);
    token.t3 = e.P(file_id);
}

void token_generator::gen_ts(const string& word, SearchToken& token) {
    encryption& e = encryption::Instance();
    token.t1 = e.F(word);
    token.t2 = e.G(word);
    token.t3 = e.P(word);
}
