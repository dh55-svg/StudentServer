#pragma once
#include <string>
#include "business_handler.h"
#include "connection_handler.h"
#include "reliable_msg_manager.h"

// 服务器类，封装所有服务器功能
class Server {
private:
    BusinessHandler business_handler_;          // 业务处理器
    ConnectionHandler* connection_handler_;     // 连接处理器
    ReliableMsgManager* reliable_msg_manager_;  // 可靠消息管理器
    bool is_running_;                           // 服务器运行状态
    int server_port_;                           // 服务器端口

public:
    // 构造函数和析构函数
    Server();
    ~Server();

    // 初始化服务器
    bool initialize(const std::string& db_host = "localhost", 
                   const std::string& db_user = "root",
                   const std::string& db_password = "123456",
                   const std::string& db_name = "students");

    // 启动服务器
    bool start(int port = 8888);

    // 停止服务器
    void stop();

    // 获取服务器运行状态
    bool isRunning() const { return is_running_; }

    // 获取服务器端口
    int getPort() const { return server_port_; }

private:
    // 初始化数据库连接
    bool initializeDatabase(const std::string& host, const std::string& user,
                           const std::string& password, const std::string& database);

    // 注册消息处理器
    void registerMessageHandlers();

    // 消息处理函数
    static void handleLoginRequest(int conn_id, const MyProtoMsg& request, MyProtoMsg& response);
    static void handleGetStudentList(int conn_id, const MyProtoMsg& request, MyProtoMsg& response);
    static void handleGetStudentDetail(int conn_id, const MyProtoMsg& request, MyProtoMsg& response);
    static void handleAddStudent(int conn_id, const MyProtoMsg& request, MyProtoMsg& response);
    static void handleUpdateStudent(int conn_id, const MyProtoMsg& request, MyProtoMsg& response);
    static void handleDeleteStudent(int conn_id, const MyProtoMsg& request, MyProtoMsg& response);
    static void handleHeartbeatRequest(int conn_id, const MyProtoMsg& request, MyProtoMsg& response);
};