#ifndef UNIQUE_WORDS_H
#define UNIQUE_WORDS_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_set>
#include <algorithm>
#include <filesystem>

using namespace std;
namespace fs = filesystem;

class word_getter {
public:
    static void process_dir(const string& dir_path, unordered_set<string>&);
    static void process_file(const string& file_name, unordered_set<string>&);
    static string extract_filename(const string&);
    static void extract_words(const string& str, unordered_set<string>&);
};


#endif