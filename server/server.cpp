#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include "../common/base64.h"
#include "../common/sse_types.h"

using boost::asio::ip::tcp;
using json = nlohmann::json;

string convert_path_for_wsl(const string& path) {
    string result = path;
    size_t pos;
    while ((pos = result.find('\\')) != string::npos) {
        result.replace(pos, 1, "/");
    }
    return result;
}

const string FILE_STORAGE_PATH = []() {
    auto path = (filesystem::current_path() / "server_storage").string();
    return convert_path_for_wsl(path) + "/";
}();

void initialize_storage() {
    try {
        filesystem::path storage_path(FILE_STORAGE_PATH);
        
        cout << "Initializing storage at: " << storage_path << endl;
        
        if (!filesystem::exists(storage_path)) {
            if (!filesystem::create_directories(storage_path)) {
                throw runtime_error("Failed to create storage directory");
            }
            cout << "Created storage directory" << endl;
        }
        
        // Проверка прав на запись
        filesystem::path test_file = storage_path / "test_write.tmp";
        {
            ofstream tmp(test_file);
            if (!tmp) throw runtime_error("No write permissions in storage directory");
            tmp << "test";
        }
        filesystem::remove(test_file);
        
        cout << "Storage initialized successfully" << endl;
    } catch (const exception& e) {
        cerr << "Storage initialization failed: " << e.what() << endl;
        throw;
    }
}

unordered_map<string, vector<uint8_t>> Ts;  // Поисковая таблица
unordered_map<string, vector<uint8_t>> Td;  // Таблица удаления
vector<tuple<vector<uint8_t>, vector<uint8_t>, vector<uint8_t>>> As;  // Поисковый массив
vector<tuple<vector<uint8_t>, vector<uint8_t>, vector<uint8_t>, vector<uint8_t>, vector<uint8_t>, vector<uint8_t>, vector<uint8_t>>> Ad;  // Массив удаления

void save_file(const string& filename, const vector<uint8_t>& data) {
    try {
        // Полный путь к файлу
        filesystem::path file_path = filesystem::path(FILE_STORAGE_PATH) / filename;
        
        // Проверка и создание поддиректорий
        filesystem::create_directories(file_path.parent_path());
        
        // Логирование перед записью
        cout << "Saving file to: " << filesystem::absolute(file_path) << endl;
        
        // Запись файла
        ofstream out(file_path, ios::binary);
        if (!out) {
            throw runtime_error("Cannot open file for writing: " + file_path.string());
        }
        out.write(reinterpret_cast<const char*>(data.data()), data.size());
        out.close();
        
        // Проверка, что файл записался
        if (!filesystem::exists(file_path)) {
            throw runtime_error("File was not created: " + file_path.string());
        }
        
        cout << "File successfully saved (" << data.size() << " bytes)" << endl;
    } catch (const exception& e) {
        cerr << "ERROR saving file: " << e.what() << endl;
        throw;
    }
}

vector<uint8_t> load_file(const string& filename) {
    ifstream in(FILE_STORAGE_PATH + filename, ios::binary);
    if (!in) return {};
    return vector<uint8_t>((istreambuf_iterator<char>(in)), 
                               istreambuf_iterator<char>());
}

json handle_add_request(const json& request) {
    try {
        string filename = request["filename"];
        vector<uint8_t> encrypted_content = Base64::decode(request["encrypted_content"].get<string>());
        
        if (filename.empty() || filename.find('/') != string::npos || 
            filename.find('\\') != string::npos) {
            return {
                {"status", "ERROR"},
                {"message", "Invalid filename"}
            };
        }

        AddToken token;
        auto token_json = request["token"];
        token.t1 = token_json["t1"].get<vector<uint8_t>>();
        token.t2 = token_json["t2"].get<vector<uint8_t>>();
        
        for (auto& lambda_json : token_json["lambdas"]) {
            LambdaTuple lambda;
            get<0>(lambda) = lambda_json["f_w"].get<vector<uint8_t>>();
            get<1>(lambda) = lambda_json["g_w"].get<vector<uint8_t>>();
            
            auto& tuple1 = get<2>(lambda);
            get<0>(tuple1) = lambda_json["tuple1"][0].get<vector<uint8_t>>();
            get<1>(tuple1) = lambda_json["tuple1"][1].get<vector<uint8_t>>();
            get<2>(tuple1) = lambda_json["tuple1"][2].get<vector<uint8_t>>();
            
            auto& tuple2 = get<3>(lambda);
            get<0>(tuple2) = lambda_json["tuple2"][0].get<vector<uint8_t>>();
            get<1>(tuple2) = lambda_json["tuple2"][1].get<vector<uint8_t>>();
            get<2>(tuple2) = lambda_json["tuple2"][2].get<vector<uint8_t>>();
            get<3>(tuple2) = lambda_json["tuple2"][3].get<vector<uint8_t>>();
            get<4>(tuple2) = lambda_json["tuple2"][4].get<vector<uint8_t>>();
            get<5>(tuple2) = lambda_json["tuple2"][5].get<vector<uint8_t>>();
            get<6>(tuple2) = lambda_json["tuple2"][6].get<vector<uint8_t>>();
            
            token.lambdas.push_back(lambda);
        }

        // Обновление структур данных
        for (auto& lambda : token.lambdas) {
            // Добавление в As
            As.push_back(get<2>(lambda));
            size_t phi = As.size() - 1;
            
            // Добавление в Ad
            Ad.push_back(get<3>(lambda));
            size_t phi_star = Ad.size() - 1;
            
            // Обновление Ts
            string f_w_key(get<0>(lambda).begin(), get<0>(lambda).end());
            Ts[f_w_key] = vector<uint8_t>((uint8_t*)&phi, (uint8_t*)&phi + sizeof(phi));
            
            // Обновление Td
            string t1_key(token.t1.begin(), token.t1.end());
            Td[t1_key] = vector<uint8_t>((uint8_t*)&phi_star, (uint8_t*)&phi_star + sizeof(phi_star));
        }

        save_file(filename, encrypted_content);

        return {
            {"status", "OK"},
            {"message", "File added successfully"}
        };

    } catch (exception& e) {
        return {
            {"status", "ERROR"},
            {"message", string("Add failed: ") + e.what()}
        };
    }
}

json handle_search_request(const json& request) {
    try {
        // Проверка структуры запроса
        if (!request.contains("token") || 
            !request["token"].is_object()) {
            return {
                {"status", "ERROR"},
                {"message", "Invalid token format"}
            };
        }

        // Извлечение бинарных данных токена
        SearchToken token;
        try {
            token.t1 = request["token"]["t1"].get<vector<uint8_t>>();
            token.t2 = request["token"]["t2"].get<vector<uint8_t>>();
            token.t3 = request["token"]["t3"].get<vector<uint8_t>>();
        } catch (const json::exception& e) {
            return {
                {"status", "ERROR"},
                {"message", string("Token parsing failed: ") + e.what()}
            };
        }

        // Поиск в Ts (бинарный ключ)
        string t1_key(token.t1.begin(), token.t1.end());
        if (Ts.empty() || Ts.find(t1_key) == Ts.end()) {
            return {
                {"status", "OK"},
                {"files", json::array()}  // Пустой результат
            };
        }

        // Обход As
        vector<string> found_files;
        try {
            const auto& ts_entry = Ts[t1_key];
            if (ts_entry.size() != sizeof(size_t)) {
                throw runtime_error("Invalid Ts entry size");
            }

            size_t addr;
            memcpy(&addr, ts_entry.data(), sizeof(addr));

            while (addr != 0 && addr < As.size()) {
                const auto& node = As[addr];
                
                if (get<0>(node).empty()) break;

                vector<uint8_t> xor_result = get<0>(node);  // id ⊕ H1(P(w))
                vector<uint8_t> h1_pw = get<1>(node);       // H1(P(w))
                vector<uint8_t> file_id = Base64::xor_vectors(xor_result, h1_pw);
                string filename(file_id.begin(), file_id.end());
                found_files.push_back(filename);

                if (get<1>(node).size() != sizeof(size_t)) break;
                memcpy(&addr, get<1>(node).data(), sizeof(addr));
            }
        } catch (const exception& e) {
            cerr << "Search traversal error: " << e.what() << endl;
            return {
                {"status", "ERROR"},
                {"message", "Search processing failed"}
            };
        }

        return {
            {"status", "OK"},
            {"files", found_files}
        };

    } catch (const exception& e) {
        cerr << "Critical search error: " << e.what() << endl;
        return {
            {"status", "ERROR"},
            {"message", "Internal server error"}
        };
    }
}

json handle_get_request(const json& request) {
    try {
        string filename = request["filename"];
        auto content = load_file(filename);
        
        if (content.empty()) {
            return {
                {"status", "ERROR"},
                {"message", "File not found"}
            };
        }
        
        return {
            {"status", "OK"},
            {"content", Base64::encode(content)}
        };
        
    } catch (exception& e) {
        return {
            {"status", "ERROR"},
            {"message", string("Get failed: ") + e.what()}
        };
    }
}

json handle_delete_request(const json& request) {
    try {
        DelToken token;
        token.t1 = request["token"]["t1"].get<vector<uint8_t>>();
        token.t2 = request["token"]["t2"].get<vector<uint8_t>>();
        token.t3 = request["token"]["t3"].get<vector<uint8_t>>();
        
        // Удаление из Td и Ad
        string t1_key(token.t1.begin(), token.t1.end());
        if (Td.count(t1_key)) {
            size_t addr;
            memcpy(&addr, Td[t1_key].data(), sizeof(addr));
            Td.erase(t1_key);
            
            while (addr != 0) {
                auto& node = Ad[addr];
                addr = *reinterpret_cast<size_t*>(get<0>(node).data());
            }
        }
        
        // Удаление файла
        string filename = request["filename"];
        filesystem::remove(FILE_STORAGE_PATH + filename);
        
        return {
            {"status", "OK"},
            {"message", "File deleted"}
        };
        
    } catch (exception& e) {
        return {
            {"status", "ERROR"},
            {"message", string("Delete failed: ") + e.what()}
        };
    }
}

json handle_request(const json& request) {
    string action = request["action"];
    
    if (action == "add") {
        return handle_add_request(request);
    }
    else if (action == "search") {
        return handle_search_request(request);
    }
    else if (action == "get") {
        return handle_get_request(request);
    }
    else if (action == "delete") {
        return handle_delete_request(request);
    }
    else {
        return {
            {"status", "ERROR"},
            {"message", "Unknown action: " + action}
        };
    }
}

void process_request(tcp::socket& socket) {
    try {
        uint32_t json_size;
        boost::asio::read(socket, boost::asio::buffer(&json_size, sizeof(json_size)));
        json_size = ntohl(json_size);

        vector<char> json_data(json_size);
        boost::asio::read(socket, boost::asio::buffer(json_data));
        
        json request = json::parse(json_data);
        cout << "Received request: " << request.dump(2) << endl;

        json response = handle_request(request);

        string response_str = response.dump();
        uint32_t response_size = htonl(response_str.size());
        boost::asio::write(socket, boost::asio::buffer(&response_size, sizeof(response_size)));
        boost::asio::write(socket, boost::asio::buffer(response_str));

    } catch (const exception& e) {
        cerr << "Error in process_request: " << e.what() << endl;
    }
}

int main() {
    try {
        initialize_storage();
        Ts.clear();
        As.clear();
        As.push_back({{}, {}, {}});
        Ad.push_back({{}, {}, {}, {}, {}, {}, {}});
        filesystem::create_directory(FILE_STORAGE_PATH);

        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 12345));

        cout << "Server started. Waiting for connections..." << endl;

        while (true) {
            tcp::socket socket(io_context);
            acceptor.accept(socket);
            cout << "Client connected: " << socket.remote_endpoint() << endl;
            process_request(socket);
            cout << "Response sent. Closing connection." << endl;
        }
    } catch (const exception& e) {
        cerr << "Server exception: " << e.what() << endl;
        return 1;
    }
    return 0;
}
