#include "unique_words.h"

void word_getter::process_dir(const string& dir_path, unordered_set<string>& unique_words) {
    try {
        fs::path target_dir = dir_path;
        
        if (!fs::exists(target_dir)) {
            cerr << "Ошибка: директория '" << dir_path << "' не существует!\n";
            return;
        }
        if (!fs::is_directory(target_dir)) {
            cerr << "Ошибка: '" << dir_path << "' не является директорией!\n";
            return;
        }

        // Перебираем все элементы в указанной директории
        for (const auto& entry : fs::directory_iterator(target_dir)) {
            if (entry.is_regular_file()) {  // Если это обычный файл (не директория)
                process_file(dir_path + entry.path().filename().string(), unique_words);
            }
        }
    } catch (const fs::filesystem_error& e) {
        cerr << "Ошибка доступа к файловой системе: " << e.what() << '\n';
    }
}

void word_getter::process_file(const string& filename, unordered_set<string>& unique_words) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Ошибка открытия файла: " << filename << endl;
        return;
    }
    
    string line;
    while (getline(file, line)) {
        extract_words(line, unique_words);
    }
    
    file.close();
}
/*
string word_getter::extract_filename(const string& path) {
    return filesystem::path(path).filename().string();
}
*/
string word_getter::extract_filename(const string& path) {
    if (filesystem::exists(path)) {
        return filesystem::path(path).filename().string();
    }
    return "";
}

void word_getter::extract_words(const string& text, unordered_set<string>& unique_words) {
    istringstream iss(text);
    string word;
    
    while (iss >> word) {
        // Удаление знаков препинания из слова
        word.erase(remove_if(word.begin(), word.end(), ::ispunct), word.end());
        // Приведение слова к нижнему регистру
        transform(word.begin(), word.end(), word.begin(), ::tolower);
        
        if (!word.empty()) {
            unique_words.insert(word);
        }
    }
}