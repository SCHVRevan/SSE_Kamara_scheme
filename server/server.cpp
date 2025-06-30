#include "server.h"

Server::Server(boost::asio::io_context& io_context, short port, const string& db_connection_str)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
      db_connector_(make_unique<DBConnector>(db_connection_str)) {
    cout << "Server initialized on port " << port << endl;
    do_accept();
}

void Server::start() {
    stopped_ = false;
    cout << "Server started" << endl;
}

void Server::stop() {
    stopped_ = true;
    acceptor_.close();
    cout << "Server stopped" << endl;
}

void Server::do_accept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                cout << "New connection from: " << socket.remote_endpoint() << endl;
                thread(&Server::handle_session, this, move(socket)).detach();
            }
            
            if (!stopped_) {
                do_accept();
            }
        });
}

json Server::receive_json(tcp::socket& socket) {
    // Читаем длину сообщения
    uint32_t length;
    boost::asio::read(socket, boost::asio::buffer(&length, sizeof(length)));
    
    // Читаем само сообщение
    vector<char> data(length);
    boost::asio::read(socket, boost::asio::buffer(data));
    
    return json::parse(string(data.begin(), data.end()));
}

void Server::send_json(tcp::socket& socket, const json& j) {
    string message = j.dump();
    uint32_t length = message.size();
    
    // Сначала отправляем длину сообщения
    boost::asio::write(socket, boost::asio::buffer(&length, sizeof(length)));
    
    // Затем само сообщение
    boost::asio::write(socket, boost::asio::buffer(message));
}

void Server::handle_session(tcp::socket socket) {
    try {
        while (true) {
            json request = receive_json(socket);
            cout << "Received request: " << request.dump(2) << endl;
            
            json response;
            
            if (request.contains("action")) {
                string action = request["action"];
                
                if (action == "add") {
                    try {
                        AddToken token = base64::deserialize_add_token(request);
                        
                        if (!request.contains("encrypted_content") || !request["encrypted_content"].is_string()) {
                            response["status"] = "error";
                            response["message"] = "Missing or invalid encrypted content";
                            send_json(socket, response);
                            continue;
                        }
                        
                        string encrypted_content = base64::decode(request["encrypted_content"].get<string>());

                        if (db_connector_->addFile(token, encrypted_content)) {
                            response["status"] = "success";
                        } else {
                            response["status"] = "error";
                            response["message"] = "Failed to add file";
                        }
                    } catch (const exception& e) {
                        response["status"] = "error";
                        response["message"] = string("Invalid add request: ") + e.what();
                    }
                }
                else if (action == "search") {
                    SearchToken token = base64::deserialize_search_token(request);
                    
                    auto file_ids = db_connector_->searchFiles(token);
                    response["status"] = "success";
                    response["files"] = file_ids;
                }
                else if (action == "delete") {
                    DelToken token = base64::deserialize_del_token(request);
                    
                    if (db_connector_->deleteFile(token)) {
                        response["status"] = "success";
                    } else {
                        response["status"] = "error";
                        response["message"] = "Failed to delete file";
                    }
                }
                else {
                    response["status"] = "error";
                    response["message"] = "Unknown action";
                }
            } else {
                response["status"] = "error";
                response["message"] = "No action specified";
            }
            
            send_json(socket, response);
        }
    } catch (const exception& e) {
        cerr << "Session error: " << e.what() << endl;
        try {
            json error_response = {{"status", "error"}, {"message", "Internal server error"}};
            send_json(socket, error_response);
        } catch (...) {
            // Игнорируем ошибки при попытке отправить ответ об ошибке
        }
    }
}