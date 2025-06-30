#include "db_connector.h"

DBConnector::DBConnector(const string& conn_str) {
    conn = make_unique<pqxx::connection>(conn_str);
}

bool DBConnector::addFile(const AddToken& token, const string& encrypted_content) {
    try {
        pqxx::work txn(*conn);
        
        // 2. Обрабатываем каждое слово из токена
        for (const auto& lambda: token.lambdas) {
            auto [f_w, g_w, tuple1, tuple2] = lambda;
            auto [h1_pw1, h1_pw2, h1_pw3] = tuple1;
            auto [h2_pf1, h2_pf2, h2_pf3, h2_pf4, h2_pf5, h2_pf6, h2_pf7] = tuple2;

            string as_node_id = "0";    // ф
            string ad_node_id = "0";    // ф*
            string last_as_node_id = "0";   // ф-1
            string last_ad_node_id = "0";   // ф*-1

            // Добавляем узел (генерируем адрес) в A_s
            pqxx::result r_as = txn.exec_params(
                "INSERT INTO A_s (file_id_xor, next_node_addr, pair_node_addr)"
                "VALUES ($1, $2, $3) RETURNING id",
                h1_pw1,
                h1_pw2,
                h1_pw3
            );
            if (r_as.empty()) {
                throw runtime_error("INSERT INTO A_s failed, no rows returned");
            }
            as_node_id = r_as[0][0].as<string>("");

            // Добавляем узел в Ad
            pqxx::result r_ad = txn.exec_params(
                "INSERT INTO Ad (next_node_d_addr, prev_node_n_addr, next_node_n_addr, pair_node_addr, prev_pair_node_addr, next_pair_node_addr, word_xor)"
                "VALUES ($1, $2, $3, $4, $5, $6, $7) RETURNING id",
                h2_pf1,
                h2_pf2,
                h2_pf3,
                h2_pf4,
                h2_pf5,
                h2_pf6,
                h2_pf7
            );
            if (r_ad.empty()) {
                throw runtime_error("INSERT INTO A_s failed, no rows returned");
            }
            ad_node_id = r_ad[0][0].as<string>("");

            // Добавляем запись в Ts
            pqxx::result ts_result = txn.exec_params(
                "SELECT first_node_addr FROM Ts WHERE encrypted_word = $1",
                f_w
            );
            
            // Если слово ещё не встречалось
            if (ts_result.empty()) {
                pqxx::result r = txn.exec_params(
                    "INSERT INTO Ts (encrypted_word, first_node_addr)"
                    "VALUES ($1, $2)",
                    f_w,
                    base64::xor_strings(as_node_id, g_w)
                );
            }
            // Если слово уже хранится
            else {
                // Преобразуем обратно в string
                string str_id = ts_result[0]["first_node_addr"].as<string>("");
                // Вычисляем адрес
                string decrypted_id = base64::xor_strings(str_id, g_w);

                // Проходим по списку, пока не дойдём до последнего элемента
                while (!decrypted_id.empty() && decrypted_id != "0") {
                    last_as_node_id = decrypted_id;
                    pqxx::result as_result = txn.exec_params(
                        "SELECT next_node_addr FROM A_s WHERE id = $1",
                        last_as_node_id
                    );

                    // Если запрос не вернул данных, выходим
                    if (as_result.empty()) {
                        cerr << "Wrong key was selected\n";
                        return false;
                    }

                    str_id = as_result[0]["next_node_addr"].as<string>("");
                    decrypted_id = base64::xor_strings(str_id, h1_pw2);
                }
            }

            if (last_as_node_id != "0") {
                // Обновляем адрес в A_s[ф-1] (шаг 2.3)
                txn.exec_params(
                    "UPDATE A_s SET next_node_addr = $1 WHERE id = $2",
                    base64::xor_strings(as_node_id, h1_pw2),
                    base64::xor_strings(last_as_node_id, h1_pw2)
                );
            }

            // Добавляем запись в Td (2.4)
            pqxx::result td_result = txn.exec_params(
                "SELECT first_node_addr FROM Td WHERE encrypted_file_id = $1",
                token.t1
            );

            // Если файл ещё не встречался
            if (td_result.empty()) {
                txn.exec_params(
                    "INSERT INTO Td (encrypted_file_id, first_node_addr)"
                    "VALUES ($1, $2)",
                    token.t1,
                    base64::xor_strings(ad_node_id, token.t2)
                );
            }
/*
            // Если файл уже хранится
            else {
                cerr << "File already exists\n";
                return false;
            }
*/
            // Если файл уже хранится
            else {
                // Преобразуем обратно в string
                string str_id = ts_result[0]["first_node_addr"].as<string>("");
                // Вычисляем адрес
                string decrypted_id = base64::xor_strings(str_id, token.t2);

                // Проходим по списку, пока не дойдём до последнего элемента
                while (!decrypted_id.empty()) {
                    last_ad_node_id = decrypted_id;
                    pqxx::result ad_result = txn.exec_params(
                        "SELECT next_node_d_addr FROM Ad WHERE id = $1",
                        last_ad_node_id
                    );

                    // Если запрос не вернул данных, выходим
                    if (ad_result.empty()) {
                        cerr << "Wrong key or file name was selected\n";
                        return false;
                    }

                    str_id = ts_result[0]["next_node_d_addr"].as<string>("");
                    decrypted_id = base64::xor_strings(str_id, h2_pf1);
                }
            }

            if (last_ad_node_id != "0") {
                // Обновляем адрес в Ad[ф*-1] (шаг 2.4)
                txn.exec_params(
                    "UPDATE Ad SET next_node_d_addr = $1 WHERE id = $2",
                    base64::xor_strings(ad_node_id, h1_pw2),
                    base64::xor_strings(last_as_node_id, h1_pw2)
                );
            }

            if (last_as_node_id != "0" && last_as_node_id != "0") {
                // Находит парный адрес ф*n-1 для элемента A_s[ф-1] и обновляет указатель next_node_n_addr элемента Ad[ф*-1] на ф*, а указатель next_pair_node_addr на ф;
                // унести в else'ы?
                pqxx::result some_result = txn.exec_params(
                    "SELECT pair_node_addr FROM A_s WHERE id = $1",
                    last_as_node_id
                );

                string str_id = some_result[0]["next_node_addr"].as<string>("");
                string decrypted_id = base64::xor_strings(str_id, h1_pw2);
                
                // 2.5
                txn.exec_params(
                    "UPDATE Ad next_node_n_addr = $1, next_pair_node_addr = $2 WHERE id = $3",
                    base64::xor_strings(ad_node_id, h2_pf1),
                    base64::xor_strings(as_node_id, h2_pf1),
                    decrypted_id
                );

                // Обновляем (2.6) 
                txn.exec_params(
                    "UPDATE Ad prev_node_n_addr = $1, pair_node_addr = $2, prev_pair_node_addr = $3 WHERE id = $4",
                    base64::xor_strings(decrypted_id, h2_pf1),
                    base64::xor_strings(as_node_id, h2_pf1),
                    base64::xor_strings(last_as_node_id, h2_pf1),
                    ad_node_id
                );
            }
            
            // Устанавливаем взаимные ссылки
            txn.exec_params(
                "UPDATE A_s SET pair_node_addr = $1 WHERE id = $2",
                base64::xor_strings(ad_node_id, h1_pw1),
                as_node_id
            );
            
            txn.exec_params(
                "UPDATE Ad SET pair_node_addr = $1 WHERE id = $2",
                base64::xor_strings(as_node_id, h2_pf1),
                ad_node_id
            );
        }

        // Добавляем зашифрованное содержимое файла
        txn.exec_params(
            "INSERT INTO sample_storage (encrypted_file_id, encrypted_content)"
            "VALUES ($1, $2) ON CONFLICT (encrypted_file_id) DO NOTHING",
            token.t1, encrypted_content
        );
                
        txn.commit();
        return true;
    } catch (const exception& e) {
        cerr << "Database error in addFile: " << e.what() << endl;
        return false;
    }
}

vector<string> DBConnector::searchFiles(const SearchToken& token) {
    vector<string> result;
    encryption& encryptor = encryption::Instance();
    string h1_pw = encryptor.H1(token.t3);
    
    try {
        pqxx::work txn(*conn);
        
        // 1. Находим первый узел в Ts
        pqxx::result ts_result = txn.exec_params(
            "SELECT first_node_addr FROM Ts WHERE encrypted_word = $1",
            token.t1
        );
        
        if (ts_result.empty()) {
            txn.commit();
            return result; // Возвращаем пустой вектор, если слово не найдено
        }
        
        // 2. Декодируем адрес первого узла
        string str_id = ts_result[0]["first_node_addr"].as<string>("");
        string current_as_node_id = base64::xor_strings(str_id, token.t2);
        
        // 3. Обходим цепочку узлов
        while (current_as_node_id != h1_pw) {
            pqxx::result as_result = txn.exec_params(
                "SELECT file_id_xor, next_node_addr FROM A_s WHERE id = $1",
                current_as_node_id
            );
            
            if (as_result.empty()) break;
            
            // 4. Декодируем file_id
            str_id = ts_result[0]["file_id_xor"].as<string>("");
            string decrypted_file_id = base64::xor_strings(str_id, h1_pw);
            
            result.push_back(decrypted_file_id);
            
            // 5. Переходим к следующему узлу
            str_id = ts_result[0]["next_node_addr"].as<string>("");
            string next_node_addr = base64::xor_strings(str_id, h1_pw);
            if (next_node_addr != h1_pw) {
                current_as_node_id = next_node_addr;
            } else {
                current_as_node_id = h1_pw;
            }
        }
        
        txn.commit();
    } catch (const exception& e) {
        cerr << "Database error in searchFiles: " << e.what() << endl;
    }
    
    return result;
}

bool DBConnector::deleteFile(const DelToken& token) {
    try {
        pqxx::work txn(*conn);
        encryption& encryptor = encryption::Instance();
        string h1_pw = encryptor.H1(token.t3);
        string h2_pf = encryptor.H2(token.t3);
        
        // 1. Находим первый узел в Td
        pqxx::result td_result = txn.exec_params(
            "SELECT first_node_addr FROM Td WHERE encrypted_file_id = $1",
            token.t1
        );
        
        if (td_result.empty()) return false;
        
        // 2. Декодируем адрес первого узла
        string str_id = td_result[0]["first_node_addr"].as<string>("");
        string current_ad_node_id = base64::xor_strings(str_id, token.t2);

        // 3. Обходим цепочку узлов в Ad
        while (current_ad_node_id != h2_pf) {
            // 3.1. Получаем данные узла
            pqxx::result ad_result = txn.exec_params(
                "SELECT next_node_d_addr, prev_node_n_addr, next_node_n_addr, pair_node_addr, prev_pair_node_addr, next_pair_node_addr, word_xor FROM Ad WHERE id = $1",
                current_ad_node_id
            );
            
            if (ad_result.empty()) break;
            
            // Получаем парный узел из A_s
            str_id = ad_result[0]["pair_node_addr"].as<string>("");
            string pair_as_node_id = base64::xor_strings(str_id, h2_pf);
            
            // 3.2. Удаляем текущий узел из Ad
            txn.exec_params(
                "DELETE FROM Ad WHERE id = $1",
                current_ad_node_id
            );

            // 3.3. Удаляем парный узел из A_s по в4
            txn.exec_params(
                "DELETE FROM A_s WHERE id = $1",
                pair_as_node_id
            );

            // Получаем в5
            str_id = ad_result[0]["prev_pair_node_addr"].as<string>("");
            string prev_pair_node_addr = base64::xor_strings(str_id, h2_pf);

            // Обновляем в A_s[в5] addr(N+1) на в6
            txn.exec_params(
                "UPDATE A_s SET next_node_addr = $1 WHERE id = $2",
                ad_result[0]["next_pair_node_addr"].as<string>(""),
                prev_pair_node_addr
            );

            // Получаем в2
            str_id = ad_result[0]["prev_node_n_addr"].as<string>("");
            string prev_node_n_addr = base64::xor_strings(str_id, h2_pf);

            // Обновляем в Ad[в2] addrd(N*+1) на в3
            txn.exec_params(
                "UPDATE Ad SET next_node_addr = $1, next_pair_node_addr = $2 WHERE id = $3",
                ad_result[0]["next_node_n_addr"].as<string>(""),
                ad_result[0]["next_pair_node_addr"].as<string>(""),
                prev_node_n_addr
            );

            // Получаем в3
            str_id = ad_result[0]["next_node_n_addr"].as<string>("");
            string next_node_n_addr = base64::xor_strings(str_id, h2_pf);

            // Обновляем в Ad[в3] addrd(N*-1) на в2
            txn.exec_params(
                "UPDATE Ad SET prev_node_n_addr = $1, prev_pair_node_addr = $2 WHERE id = $3",
                ad_result[0]["prev_node_n_addr"].as<string>(""),
                ad_result[0]["prev_pair_node_addr"].as<string>(""),
                next_node_n_addr
            );
            
            // 3.5. Переходим к следующему узлу
            str_id = ad_result[0]["next_node_d_addr"].as<string>("");
            string next_node_d_addr = base64::xor_strings(str_id, h2_pf);

            if (next_node_d_addr != h2_pf) {
                current_ad_node_id = next_node_d_addr;
            } else {
                current_ad_node_id = h2_pf;
            }
        }
        
        // 4. Удаляем запись из Td
        txn.exec_params(
            "DELETE FROM Td WHERE encrypted_file_id = $1",
            token.t1
        );
        
        // 5. Удаляем содержимое файла
        txn.exec_params(
            "DELETE FROM sample_storage WHERE encrypted_file_id = $1",
            token.t1
        );
        
        txn.commit();
        return true;
    } catch (const exception& e) {
        cerr << "Database error in deleteFile: " << e.what() << endl;
        return false;
    }
}