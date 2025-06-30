#ifndef SERVER_H
#define SERVER_H

#include <boost/asio.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include "db_connector.h"
#include "../common/sse_types.h"
#include "../common/base64.h"

using boost::asio::ip::tcp;
using json = nlohmann::json;
using namespace std;

class Server {
    void do_accept();
    void handle_session(tcp::socket socket);
    
    json receive_json(tcp::socket& socket);
    void send_json(tcp::socket& socket, const json& j);
    
    std::unique_ptr<DBConnector> db_connector_;
    tcp::acceptor acceptor_;
    bool stopped_ = false;
public:
    Server(boost::asio::io_context& io_context, short port, const std::string& db_connection_str);
    void start();
    void stop();
};

#endif