#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include "../common/base64.h"
#include "encryption.h"
#include "token_gen.h"
#include "../common/sse_types.h"

using boost::asio::ip::tcp;
using json = nlohmann::json;

void showMenu() {
    cout << "\nAvailable commands:\n"
         << "1. add <file_path> - Add a file\n"
         << "2. search <word> - Search for a word\n"
         << "3. get <filename> - Get a file\n"
         << "4. delete <filename> - Delete a file\n"
         << "5. exit - Exit program\n"
         << "Enter command: ";
}

json send_request(const json& request) {
    try {
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        tcp::socket socket(io_context);

        boost::asio::connect(socket, resolver.resolve("127.0.0.1", "12345"));

        string request_str = request.dump();
        uint32_t request_size = htonl(request_str.size());
        boost::asio::write(socket, boost::asio::buffer(&request_size, sizeof(request_size)));
        boost::asio::write(socket, boost::asio::buffer(request_str));

        uint32_t response_size;
        boost::asio::read(socket, boost::asio::buffer(&response_size, sizeof(response_size)));
        response_size = ntohl(response_size);

        vector<char> response_data(response_size);
        boost::asio::read(socket, boost::asio::buffer(response_data));

        return json::parse(response_data.begin(), response_data.end());

    } catch (const exception& e) {
        cerr << "Network error: " << e.what() << '\n';
        return {{"status", "ERROR"}, {"message", "Network error"}};
    }
}

void add_file(const string& filename) {
    try {
        ifstream file(filename, ios::binary);
        if (!file) throw runtime_error("Cannot open file: " + filename);
        string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        
        // Выделение имени файла из полученного пути
        filesystem::path path_obj(filename);
        string filename = path_obj.filename().string();

        AddToken add_token;
        token_generator::gen_ta(filename, content, add_token);
        
        vector<uint8_t> encrypted = encryption::Instance().encrypt_files(vector<uint8_t>(content.begin(), content.end()));
        
        // Формирование запроса к серверу
        json request;
        request["action"] = "add";
        request["filename"] = filename;
        request["encrypted_content"] = Base64::encode(encrypted);
        
        json token_json;
        token_json["t1"] = add_token.t1;
        token_json["t2"] = add_token.t2;
        token_json["lambdas"] = json::array();
        
        for (const auto& lambda : add_token.lambdas) {
            json lambda_json;
            lambda_json["f_w"] = get<0>(lambda);
            lambda_json["g_w"] = get<1>(lambda);
            
            const auto& tuple1 = get<2>(lambda);
            lambda_json["tuple1"] = {
                get<0>(tuple1),
                get<1>(tuple1),
                get<2>(tuple1)
            };
            
            const auto& tuple2 = get<3>(lambda);
            lambda_json["tuple2"] = {
                get<0>(tuple2),
                get<1>(tuple2),
                get<2>(tuple2),
                get<3>(tuple2),
                get<4>(tuple2),
                get<5>(tuple2),
                get<6>(tuple2)
            };
            
            token_json["lambdas"].push_back(lambda_json);
        }
        
        request["token"] = token_json;

        json response = send_request(request);
        cout << "Add file response: " << response.dump(2) << '\n';

    } catch (const exception& e) {
        cerr << "Error adding file: " << e.what() << '\n';
    }
}

void search_word(const string& word) {
    try {
        SearchToken token;
        token_generator::gen_ts(word, token);
        
        json request;
        request["action"] = "search";
        request["token"] = {
            {"t1", token.t1},
            {"t2", token.t2},
            {"t3", token.t3}
        };
        
        json response = send_request(request);
        if (response["status"] == "OK") {
            cout << "Search results for '" << word << "':\n";
            for (const auto& file : response["files"]) {
                string filename = file.get<string>();
                cout << " - " << filename << '\n';
            }
        } else {
            cerr << "Search failed: " << response["message"] << '\n';
        }
    } catch (const exception& e) {
        cerr << "Search error: " << e.what() << '\n';
    }
}

void get_file(const string& filename) {
    try {
        json request;
        request["action"] = "get";
        request["filename"] = filename;
        
        json response = send_request(request);
        
        if (response["status"] == "OK") {
            vector<uint8_t> encrypted = Base64::decode(response["content"].get<string>());
            auto decrypted = encryption::Instance().decrypt_files(encrypted);
            
            cout << "File content (" << decrypted.size() << " bytes):\n";
            cout << string(decrypted.begin(), decrypted.end()) << '\n';
        } else {
            cerr << "Error: " << response["message"] << '\n';
        }
    } catch (const exception& e) {
        cerr << "Get file error: " << e.what() << '\n';
    }
}

void delete_file(const string& filename) {
    try {
        json request;
        request["action"] = "delete";
        request["filename"] = filename;

        json response = send_request(request);
        cout << "Server response: " << response.dump(4) << '\n';

    } catch (const exception& e) {
        cerr << "Error deleting file: " << e.what() << '\n';
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <encryption_key>\n";
        return 1;
    }

    encryption::Instance().setKey(argv[1]);
    cout << "Client started. Type 'help' for commands.\n";

    while (true) {
        showMenu();
        string input;
        getline(cin, input);

        if (input.empty()) continue;

        // Разделение команды и аргумента
        size_t space_pos = input.find(' ');
        string command = input.substr(0, space_pos);
        string argument = space_pos != string::npos ? input.substr(space_pos + 1) : "";

        if (command == "add" && !argument.empty()) {
            add_file(argument);
        } 
        else if (command == "search" && !argument.empty()) {
            search_word(argument);
        }
        else if (command == "get" && !argument.empty()) {
            get_file(argument);
        }
        else if (command == "delete" && !argument.empty()) {
            delete_file(argument);
        }
        else if (command == "exit") {
            break;
        }
        else {
            cout << "Unknown command or missing argument.\n";
        }
    }

    return 0;
}
