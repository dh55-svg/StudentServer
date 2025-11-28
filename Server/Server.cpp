#include "Server.h"
#include <iostream>
#include <thread>
#include <chrono>
#include "DatabaseManager.h"

Server::Server() : connection_handler_(nullptr), reliable_msg_manager_(nullptr), is_running_(false), server_port_(0) {
}

Server::~Server() {
    stop();
    if (reliable_msg_manager_) {
        delete reliable_msg_manager_;
        reliable_msg_manager_ = nullptr;
    }
}

// 在initialize方法中添加初始化代码
bool Server::initialize(const std::string& db_host, const std::string& db_user,
                       const std::string& db_password, const std::string& db_name) {
    std::cout << "服务器初始化中..." << std::endl;

    // 初始化数据库
    if (!initializeDatabase(db_host, db_user, db_password, db_name)) {
        return false;
    }

    enhanced_business_handler_ = EnhancedBusinessHandler::getInstance();
    enhanced_business_handler_->initialize(&business_handler_);

    // 创建可靠消息管理器
    reliable_msg_manager_ = new ReliableMsgManager(3, 2000); // 最大3次重传，2秒间隔

    // 获取连接处理器实例
    connection_handler_ = ConnectionHandler::getInstance();
    if (!connection_handler_) {
        std::cerr << "获取连接处理器失败" << std::endl;
        return false;
    }

    std::cout << "服务器初始化完成" << std::endl;
    return true;
}

bool Server::start(int port) {
    if (is_running_) {
        std::cout << "服务器已经在运行中" << std::endl;
        return true;
    }

    if (!connection_handler_) {
        std::cerr << "服务器未初始化" << std::endl;
        return false;
    }

    // 初始化连接处理器
    if (!connection_handler_->initialize(enhanced_business_handler_->getBusinessHandler(), reliable_msg_manager_)) {
        std::cerr << "连接处理器初始化失败" << std::endl;
        return false;
    }

    // 设置心跳配置
    connection_handler_->setHeartbeatConfig(3000, 60000); // 3秒心跳，60秒超时

    // 启动服务器
    if (!connection_handler_->startServer(port)) {
        std::cerr << "服务器启动失败，端口：" << port << std::endl;
        return false;
    }

    server_port_ = port;
    is_running_ = true;

    std::cout << "服务器启动成功，正在监听端口：" << port << std::endl;
    return true;
}

void Server::stop() {
    if (!is_running_ || !connection_handler_) {
        return;
    }

    std::cout << "服务器正在停止..." << std::endl;

    // 停止超时检测
    if (reliable_msg_manager_) {
        reliable_msg_manager_->stopTimeoutCheck();
    }

    // 停止服务器
    connection_handler_->stopServer();

    is_running_ = false;
    server_port_ = 0;

    std::cout << "服务器已停止" << std::endl;
}

bool Server::initializeDatabase(const std::string& host, const std::string& user,
                               const std::string& password, const std::string& database) {
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    
    bool connected = dbManager->connect(host, user, password, database);
    
    if (connected) {
        std::cout << "数据库连接成功" << std::endl;
        return true;
    } else {
        std::cerr << "数据库连接失败" << std::endl;
        return false;
    }
}

void Server::registerMessageHandlers() {
    
    
    std::cout << "所有消息处理器注册完成" << std::endl;
}

void Server::handleLoginRequest(int conn_id, const MyProtoMsg& request, MyProtoMsg& response) {
    try {
        std::string username = request.body["username"];
        std::string password = request.body["password"];
        
        // 使用DatabaseManager进行用户认证
        DatabaseManager* dbManager = DatabaseManager::getInstance();
        json result = dbManager->authenticateUser(username, password);
        
        if (result["success"] == true) {
            // 获取用户权限
            json permissions = dbManager->getUserPermissions(username);
            response.body["status"] = "success";
            response.body["message"] = "登录成功";
            response.body["user"] = result["user"]["username"];
            response.body["permissions"] = permissions;
        } else {
            response.body["status"] = "error";
            response.body["message"] = result["message"].get<std::string>();
        }
    } catch (const std::exception& e) {
        response.body["status"] = "error";
        response.body["message"] = "处理登录请求时发生错误";
        std::cerr << "Login request error: " << e.what() << std::endl;
    }
}

void Server::handleGetStudentList(int conn_id, const MyProtoMsg& request, MyProtoMsg& response) {
    try {
        DatabaseManager* dbManager = DatabaseManager::getInstance();
        json studentList = dbManager->getStudentList();
        
        response.body["status"] = "success";
        response.body["students"] = studentList;
    } catch (const std::exception& e) {
        response.body["status"] = "error";
        response.body["message"] = "获取学生列表失败";
        std::cerr << "Get student list error: " << e.what() << std::endl;
    }
}

void Server::handleGetStudentDetail(int conn_id, const MyProtoMsg& request, MyProtoMsg& response) {
    try {
        std::string studentId = request.body["studentId"];
        DatabaseManager* dbManager = DatabaseManager::getInstance();
        json studentDetail = dbManager->getStudentDetail(studentId);
        
        if (!studentDetail.is_null()) {
            response.body["status"] = "success";
            response.body["student"] = studentDetail;
        } else {
            response.body["status"] = "error";
            response.body["message"] = "学生不存在";
        }
    } catch (const std::exception& e) {
        response.body["status"] = "error";
        response.body["message"] = "获取学生详情失败";
        std::cerr << "Get student detail error: " << e.what() << std::endl;
    }
}

void Server::handleAddStudent(int conn_id, const MyProtoMsg& request, MyProtoMsg& response) {
    try {
        json studentData = request.body["studentData"];
        DatabaseManager* dbManager = DatabaseManager::getInstance();
        
        if (dbManager->addStudent(studentData)) {
            response.body["status"] = "success";
            response.body["message"] = "学生添加成功";
        } else {
            response.body["status"] = "error";
            response.body["message"] = "学生添加失败";
        }
    } catch (const std::exception& e) {
        response.body["status"] = "error";
        response.body["message"] = "添加学生时发生错误";
        std::cerr << "Add student error: " << e.what() << std::endl;
    }
}

void Server::handleUpdateStudent(int conn_id, const MyProtoMsg& request, MyProtoMsg& response) {
    try {
        std::string studentId = request.body["studentId"];
        json studentData = request.body["studentData"];
        DatabaseManager* dbManager = DatabaseManager::getInstance();
        
        if (dbManager->updateStudent(studentId, studentData)) {
            response.body["status"] = "success";
            response.body["message"] = "学生信息更新成功";
        } else {
            response.body["status"] = "error";
            response.body["message"] = "学生信息更新失败";
        }
    } catch (const std::exception& e) {
        response.body["status"] = "error";
        response.body["message"] = "更新学生信息时发生错误";
        std::cerr << "Update student error: " << e.what() << std::endl;
    }
}

void Server::handleDeleteStudent(int conn_id, const MyProtoMsg& request, MyProtoMsg& response) {
    try {
        std::string studentId = request.body["studentId"];
        DatabaseManager* dbManager = DatabaseManager::getInstance();
        
        if (dbManager->deleteStudent(studentId)) {
            response.body["status"] = "success";
            response.body["message"] = "学生删除成功";
        } else {
            response.body["status"] = "error";
            response.body["message"] = "学生删除失败";
        }
    } catch (const std::exception& e) {
        response.body["status"] = "error";
        response.body["message"] = "删除学生时发生错误";
        std::cerr << "Delete student error: " << e.what() << std::endl;
    }
}

void Server::handleHeartbeatRequest(int conn_id, const MyProtoMsg& request, MyProtoMsg& response) {
    response.body["status"] = "success";
    response.body["timestamp"] = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}