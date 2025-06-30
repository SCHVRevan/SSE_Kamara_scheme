#ifndef DB_CONNECTOR_H
#define DB_CONNECTOR_H

#include <pqxx/pqxx>
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include "../client/encryption.h"
#include "../common/sse_types.h"
#include "../common/base64.h"

using namespace std;

class DBConnector {
    unique_ptr<pqxx::connection> conn;
    string xor_encrypt(int value, const string& key);

public:
    DBConnector(const string& conn_str);
    
    bool addFile(const AddToken& token, const string& encrypted_content);
    bool deleteFile(const DelToken& token);
    vector<string> searchFiles(const SearchToken& token);
};

#endif