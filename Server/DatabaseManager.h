#pragma once
#include <string>
#include <vector>
#include <mysql/mysql.h>
#include <mutex>
#include "json.hpp"
using json = nlohmann::json;
using namespace std;
class DatabaseManager{
private:
  MYSQL* connection_;
  static DatabaseManager* instance_;
  DatabaseManager();
  bool initializeConnection(const string& host,const string& user,const string& password,const string& database);
  static mutex mutex_;
public:
  static DatabaseManager*getInstance();
  ~DatabaseManager();
  bool connect(const std::string& host = "localhost", const std::string& user = "root", 
                const std::string& password = "", const std::string& database = "students");
  void disconnect();
  bool isConnected() const;
  json executeQuery(const string& query,const vector<string>& params={});
  int executeUpdate(const string& query,const vector<string>& params={});

  // 事务支持
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    
    // 特定功能的安全查询方法
    //认证用户
    json authenticateUser(const std::string& username, const std::string& password);
    json getUserPermissions(const std::string& username);
    json getStudentList();
    json searchStudent(const std::string& keyword);
    json getStudentDetail(const std::string& studentId);
    bool addStudent(const json& studentData);
    bool updateStudent(const std::string& studentId, const json& studentData);
    bool deleteStudent(const std::string& studentId);
};