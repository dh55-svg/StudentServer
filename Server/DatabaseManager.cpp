#include "DatabaseManager.h"
#include <iostream>
#include <sstream>

DatabaseManager* DatabaseManager::instance_ = nullptr;
mutex DatabaseManager::mutex_;

DatabaseManager::DatabaseManager() : connection_(nullptr) {
    // 初始化MySQL库
    mysql_library_init(0, nullptr, nullptr);
    connection_ = mysql_init(nullptr);
    if (connection_ == nullptr) {
        cerr << "MySQL initialization failed" << endl;
    }
}

DatabaseManager::~DatabaseManager() {
    disconnect();
    mysql_library_end();
}

DatabaseManager* DatabaseManager::getInstance() {
    if (instance_ == nullptr) {
        lock_guard<mutex> lock(mutex_);
        if (instance_ == nullptr) {
            instance_ = new DatabaseManager();
        }
    }
    return instance_;
}

bool DatabaseManager::initializeConnection(const string& host, const string& user, const string& password, const string& database) {
    if (connection_ == nullptr) {
        connection_ = mysql_init(nullptr);
        if (connection_ == nullptr) {
            return false;
        }
    }
    
    // 设置连接选项
    mysql_options(connection_, MYSQL_SET_CHARSET_NAME, "utf8mb4");
    
    if (!mysql_real_connect(connection_, host.c_str(), user.c_str(), password.c_str(), 
                           database.c_str(), 3306, nullptr, 0)) {
        cerr << "MySQL connection failed: " << mysql_error(connection_) << endl;
        return false;
    }
    
    return true;
}

bool DatabaseManager::connect(const string& host, const string& user, const string& password, const string& database) {
    return initializeConnection(host, user, password, database);
}

void DatabaseManager::disconnect() {
    if (connection_ != nullptr) {
        mysql_close(connection_);
        connection_ = nullptr;
    }
}

bool DatabaseManager::isConnected() const {
    return connection_ != nullptr && mysql_ping(connection_) == 0;
}

json DatabaseManager::executeQuery(const string& query, const vector<string>& params) {
    json result;
    
    if (!isConnected()) {
        cerr << "Not connected to database" << endl;
        return result;
    }
    
    // 创建预处理语句
    MYSQL_STMT* stmt = mysql_stmt_init(connection_);
    if (stmt == nullptr) {
        cerr << "mysql_stmt_init failed" << endl;
        return result;
    }
    
    // 准备预处理语句
    if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
        cerr << "mysql_stmt_prepare failed: " << mysql_stmt_error(stmt) << endl;
        mysql_stmt_close(stmt);
        return result;
    }
    
    // 绑定参数
    unsigned long param_count = mysql_stmt_param_count(stmt);
    if (param_count != params.size()) {
        cerr << "Parameter count mismatch: expected " << param_count << ", got " << params.size() << endl;
        mysql_stmt_close(stmt);
        return result;
    }
    
    if (param_count > 0) {
        vector<MYSQL_BIND> bind(param_count);
        vector<unsigned long> lengths(param_count);
        
        for (unsigned int i = 0; i < param_count; ++i) {
            memset(&bind[i], 0, sizeof(MYSQL_BIND));
            bind[i].buffer_type = MYSQL_TYPE_STRING;
            bind[i].buffer = const_cast<char*>(params[i].c_str());
            bind[i].buffer_length = params[i].length();
            lengths[i] = params[i].length();
            bind[i].length = &lengths[i];
        }
        
        if (mysql_stmt_bind_param(stmt, bind.data()) != 0) {
            cerr << "mysql_stmt_bind_param failed: " << mysql_stmt_error(stmt) << endl;
            mysql_stmt_close(stmt);
            return result;
        }
    }
    
    // 执行查询
    if (mysql_stmt_execute(stmt) != 0) {
        cerr << "mysql_stmt_execute failed: " << mysql_stmt_error(stmt) << endl;
        mysql_stmt_close(stmt);
        return result;
    }
    
    // 获取结果集元数据
    MYSQL_RES* meta_result = mysql_stmt_result_metadata(stmt);
    if (meta_result == nullptr) {
        // 可能是UPDATE/DELETE等没有结果集的语句
        mysql_stmt_close(stmt);
        return result;
    }
    
    int num_fields = mysql_num_fields(meta_result);
    vector<string> field_names;
    
    // 获取字段名
    MYSQL_FIELD* fields = mysql_fetch_fields(meta_result);
    for (int i = 0; i < num_fields; ++i) {
        field_names.push_back(fields[i].name);
    }
    
    // 准备结果集绑定
    vector<MYSQL_BIND> bind(num_fields);
    vector<char*> buffers(num_fields);
    vector<unsigned long> lengths(num_fields);
    vector<unsigned char> is_null(num_fields);
    
    for (int i = 0; i < num_fields; ++i) {
        memset(&bind[i], 0, sizeof(MYSQL_BIND));
        bind[i].buffer_type = MYSQL_TYPE_STRING;
        bind[i].is_null = reinterpret_cast<bool *>(&is_null[i]);
        bind[i].length = &lengths[i];
        // 分配足够的空间
        buffers[i] = new char[4096];
        bind[i].buffer = buffers[i];
        bind[i].buffer_length = 4096;
    }
    
    if (mysql_stmt_bind_result(stmt, bind.data()) != 0) {
        cerr << "mysql_stmt_bind_result failed: " << mysql_stmt_error(stmt) << endl;
        mysql_free_result(meta_result);
        mysql_stmt_close(stmt);
        // 释放缓冲区
        for (auto& buffer : buffers) {
            delete[] buffer;
        }
        return result;
    }
    
    // 获取结果
    while (mysql_stmt_fetch(stmt) == 0) {
        json row;
        for (int i = 0; i < num_fields; ++i) {
            if (is_null[i]) {
                row[field_names[i]] = nullptr;
            } else {
                row[field_names[i]] = string(buffers[i], lengths[i]);
            }
        }
        result.push_back(row);
    }
    
    // 清理资源
    mysql_free_result(meta_result);
    mysql_stmt_close(stmt);
    // 释放缓冲区
    for (auto& buffer : buffers) {
        delete[] buffer;
    }
    
    return result;
}

int DatabaseManager::executeUpdate(const string& query, const vector<string>& params) {
    if (!isConnected()) {
        cerr << "Not connected to database" << endl;
        return -1;
    }
    
    // 创建预处理语句
    MYSQL_STMT* stmt = mysql_stmt_init(connection_);
    if (stmt == nullptr) {
        cerr << "mysql_stmt_init failed" << endl;
        return -1;
    }
    
    // 准备预处理语句
    if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
        cerr << "mysql_stmt_prepare failed: " << mysql_stmt_error(stmt) << endl;
        mysql_stmt_close(stmt);
        return -1;
    }
    
    // 绑定参数
    unsigned long param_count = mysql_stmt_param_count(stmt);
    if (param_count != params.size()) {
        cerr << "Parameter count mismatch: expected " << param_count << ", got " << params.size() << endl;
        mysql_stmt_close(stmt);
        return -1;
    }
    
    if (param_count > 0) {
        vector<MYSQL_BIND> bind(param_count);
        vector<unsigned long> lengths(param_count);
        
        for (unsigned int i = 0; i < param_count; ++i) {
            memset(&bind[i], 0, sizeof(MYSQL_BIND));
            bind[i].buffer_type = MYSQL_TYPE_STRING;
            bind[i].buffer = const_cast<char*>(params[i].c_str());
            bind[i].buffer_length = params[i].length();
            lengths[i] = params[i].length();
            bind[i].length = &lengths[i];
        }
        
        if (mysql_stmt_bind_param(stmt, bind.data()) != 0) {
            cerr << "mysql_stmt_bind_param failed: " << mysql_stmt_error(stmt) << endl;
            mysql_stmt_close(stmt);
            return -1;
        }
    }
    
    // 执行更新
    if (mysql_stmt_execute(stmt) != 0) {
        cerr << "mysql_stmt_execute failed: " << mysql_stmt_error(stmt) << endl;
        mysql_stmt_close(stmt);
        return -1;
    }
    
    // 获取受影响的行数
    my_ulonglong affected_rows = mysql_stmt_affected_rows(stmt);
    
    // 清理资源
    mysql_stmt_close(stmt);
    
    return static_cast<int>(affected_rows);
}

// 事务支持方法
bool DatabaseManager::beginTransaction() {
    if (!isConnected()) return false;
    return mysql_query(connection_, "START TRANSACTION") == 0;
}

bool DatabaseManager::commitTransaction() {
    if (!isConnected()) return false;
    return mysql_query(connection_, "COMMIT") == 0;
}

bool DatabaseManager::rollbackTransaction() {
    if (!isConnected()) return false;
    return mysql_query(connection_, "ROLLBACK") == 0;
}

// 特定功能的安全查询方法
json DatabaseManager::authenticateUser(const string& username, const string& password) {
    vector<string> params = {username, password};
    string query = "SELECT * FROM user WHERE username = ? AND password = ?";
    return executeQuery(query, params);
}

json DatabaseManager::getUserPermissions(const string& username) {
    vector<string> params = {username};
    string query = "SELECT * FROM user WHERE username = ?";
    json result = executeQuery(query, params);
    if (!result.empty()) {
        return result[0];
    }
    return json();
}

json DatabaseManager::getStudentList() {
    vector<string> params;
    return executeQuery("SELECT * FROM studentinfo ORDER BY number", params);
}

json DatabaseManager::searchStudent(const string& keyword) {
    vector<string> params = {"%" + keyword + "%", "%" + keyword + "%"};
    string query = "SELECT * FROM studentinfo WHERE name LIKE ? OR number LIKE ?";
    return executeQuery(query, params);
}

json DatabaseManager::getStudentDetail(const string& studentId) {
    vector<string> params = {studentId};
    string query = "SELECT * FROM studentinfo WHERE number = ?";
    json result = executeQuery(query, params);
    if (!result.empty()) {
        return result[0];
    }
    return json();
}

bool DatabaseManager::addStudent(const json& studentData) {
    vector<string> params = {
        studentData.value("name", ""),
        studentData.value("number", ""),
        studentData.value("sex", ""),
        studentData.value("nation", ""),
        studentData.value("political", ""),
        studentData.value("birthData", ""),
        studentData.value("birthPlace", ""),
        studentData.value("idCard", ""),
        studentData.value("province", ""),
        studentData.value("city", ""),
        studentData.value("university", ""),
        studentData.value("college", ""),
        studentData.value("profession", ""),
        studentData.value("status", ""),
        studentData.value("dataOfAdmission", ""),
        studentData.value("dataOfGraduation", ""),
        studentData.value("homeAddress", ""),
        studentData.value("phone", ""),
        studentData.value("socialStatus", ""),
        studentData.value("blood", ""),
        studentData.value("eye", ""),
        studentData.value("skin", ""),
        studentData.value("fatherName", ""),
        studentData.value("fatherWork", ""),
        studentData.value("motherName", ""),
        studentData.value("motherWork", ""),
        studentData.value("parentOtherInformation", ""),
        studentData.value("photo", ""),
        studentData.value("otherInterest", "")
    };
    
    string query = "INSERT INTO studentinfo (name, number, sex, nation, political, birthData, birthPlace, idCard, "
                  "province, city, university, college, profession, status, dataOfAdmission, dataOfGraduation, "
                  "homeAddress, phone, socialStatus, blood, eye, skin, fatherName, fatherWork, motherName, motherWork, "
                  "parentOtherInformation, photo, otherInterest) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    
    int rows = executeUpdate(query, params);
    return rows > 0;
}

bool DatabaseManager::updateStudent(const string& studentId, const json& studentData) {
    vector<string> params;
    string query = "UPDATE studentinfo SET ";
    bool first = true;
    
    // 动态构建SET子句
    for (auto& [key, value] : studentData.items()) {
        if (key == "number") continue; // 不允许修改学号
        
        if (!first) query += ", ";
        query += key + " = ?";
        params.push_back(value.is_string() ? value.get<string>() : value.dump());
        first = false;
    }
    
    query += " WHERE number = ?";
    params.push_back(studentId);
    
    int rows = executeUpdate(query, params);
    return rows > 0;
}

bool DatabaseManager::deleteStudent(const string& studentId) {
    vector<string> params = {studentId};
    string query = "DELETE FROM studentinfo WHERE number = ?";
    int rows = executeUpdate(query, params);
    return rows > 0;
}