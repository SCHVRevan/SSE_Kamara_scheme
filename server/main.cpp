#include <boost/asio.hpp>
#include <memory>
#include "server.h"
#include "../common/config.h"

int main() {
    try {
        boost::asio::io_context io_context;
        string db_conn_str = "host=" + db_host + " dbname=" + db_name + " user=" + db_user + " password=" + db_pwd;
        
        Server server(io_context, 12345, db_conn_str);
        server.start();
        
        io_context.run();
    } catch (const exception& e) {
        cerr << "Server exception: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}