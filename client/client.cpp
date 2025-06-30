#include <iostream>
#include <boost/asio.hpp>
#include "encryption.h"
#include "unique_words.h"
#include "cipher.h"
#include "token_gen.h"
#include "../common/base64.h"

using boost::asio::ip::tcp;
using json = nlohmann::json;
using namespace std;

json receiveJson(tcp::socket& socket) {
    uint32_t length;
    boost::asio::read(socket, boost::asio::buffer(&length, sizeof(length)));
    
    vector<char> data(length);
    boost::asio::read(socket, boost::asio::buffer(data));
    
    return json::parse(string(data.begin(), data.end()));
}

void sendJson(tcp::socket& socket, const json& j) {
    string message = j.dump();
    uint32_t length = message.size();
    
    boost::asio::write(socket, boost::asio::buffer(&length, sizeof(length)));
    boost::asio::write(socket, boost::asio::buffer(message));
}

void handleAddFile(tcp::socket& socket, const string& file_path) {
    encryption& e = encryption::Instance();
    
    AddToken token;
    token_generator::gen_ta(file_path, token);
    
    json request = base64::serialize(token);
    
    // Шифруем, кодируем и добавляем содержимое файла
    string encrypted_content = e.encrypt_files(file_path);
    request["encrypted_content"] = base64::encode(encrypted_content); // Кодируем в Base64
    
    sendJson(socket, request);
    
    // Получаем ответ от сервера
    json response = receiveJson(socket);
    cout << "Server response: " << response.dump(2) << endl;
}

void handleSearch(tcp::socket& socket, const string& word) {
    encryption& e = encryption::Instance();
    
    SearchToken token;
    token_generator::gen_ts(word, token);
    
    json request = base64::serialize(token);
    
    sendJson(socket, request);
    
    // Получаем ответ от сервера
    json response = receiveJson(socket);
    if (response["status"] == "success") {
        cout << "Found files: " << response["files"].dump() << endl;
    } else {
        cerr << "Search failed: " << response["message"] << endl;
    }
}

void handleDeleteFile(tcp::socket& socket, const string& file_path) {
    encryption& e = encryption::Instance();
    
    DelToken token;
    token_generator::gen_td(file_path, token);
    
    json request = base64::serialize(token);
    
    sendJson(socket, request);
    
    // Получаем ответ от сервера
    json response = receiveJson(socket);
    if (response["status"] == "success") {
        cout << "File deleted successfully" << endl;
    } else {
        cerr << "Deletion failed: " << response["message"] << endl;
    }
}

void showMenu() {
    cout << "\nAvailable commands:\n"
         << "1. add <file_path> - Add a file\n"
         << "2. search <word> - Search for a word\n"
         << "3. delete <file_path> - Delete a file\n"
         << "4. exit - Exit the program\n";
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <encryption_key>\n";
        return 1;
    }

    string KEY = argv[1];
    encryption& e = encryption::Instance();
    e.setKey(KEY);

    try {
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        tcp::resolver::results_type endpoints = resolver.resolve("127.0.0.1", "12345");
        
        tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints);

        cout << "Connected to server\n";
        showMenu();

        while (true) {
            cout << "\nEnter command: ";
            string input;
            getline(cin, input);

            if (input.empty()) continue;

            // Разбираем ввод пользователя
            size_t space_pos = input.find(' ');
            string command = input.substr(0, space_pos);
            string argument = space_pos != string::npos ? input.substr(space_pos + 1) : "";

            if (command == "add" && !argument.empty()) {
                handleAddFile(socket, argument);
            } 
            else if (command == "search" && !argument.empty()) {
                handleSearch(socket, argument);
            }
            else if (command == "delete" && !argument.empty()) {
                handleDeleteFile(socket, argument);
            }
            else if (command == "exit") {
                break;
            }
            else {
                cerr << "Invalid command. Try again.\n";
                showMenu();
            }
        }
    } 
    catch (const exception& e) {
        cerr << "Exception: " << e.what() << endl;
        return 1;
    }

    return 0;
}