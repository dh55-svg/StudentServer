#include "EnhancedBusinessHandler.h"
#include "MyProto.h"
#include <iostream>
#include "json.hpp"

using json = nlohmann::json;

EnhancedBusinessHandler* EnhancedBusinessHandler::instance = nullptr;

EnhancedBusinessHandler::EnhancedBusinessHandler() {
    userService = UserService::getInstance();
    studentService = StudentService::getInstance();
    businessHandler = nullptr;
}

EnhancedBusinessHandler* EnhancedBusinessHandler::getInstance() {
    if (!instance) {
        instance = new EnhancedBusinessHandler();
    }
    return instance;
}

EnhancedBusinessHandler::~EnhancedBusinessHandler() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}

void EnhancedBusinessHandler::initialize(BusinessHandler* handler) {
    businessHandler = handler;
    
    if (handler) {
        // 注册所有消息处理器
        handler->registerHandler(1001, std::bind(&EnhancedBusinessHandler::handleLogin, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        handler->registerHandler(1002, std::bind(&EnhancedBusinessHandler::handleGetUserPermissions, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        handler->registerHandler(2001, std::bind(&EnhancedBusinessHandler::handleGetStudentList, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        handler->registerHandler(2002, std::bind(&EnhancedBusinessHandler::handleSearchStudent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        handler->registerHandler(2003, std::bind(&EnhancedBusinessHandler::handleGetStudentDetail, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        handler->registerHandler(2004, std::bind(&EnhancedBusinessHandler::handleAddStudent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        handler->registerHandler(2005, std::bind(&EnhancedBusinessHandler::handleUpdateStudent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        handler->registerHandler(2006, std::bind(&EnhancedBusinessHandler::handleDeleteStudent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        handler->registerHandler(1003, std::bind(&EnhancedBusinessHandler::handleAddUser, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        handler->registerHandler(1004, std::bind(&EnhancedBusinessHandler::handleUpdateUser, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        handler->registerHandler(1005, std::bind(&EnhancedBusinessHandler::handleDeleteUser, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        handler->registerHandler(1006, std::bind(&EnhancedBusinessHandler::handleGetAllUsers, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    }
}

bool EnhancedBusinessHandler::forwardMessage(int conn_id, MyProtoMsg& msg) {
    if (businessHandler) {
        return businessHandler->handleMessage(conn_id, msg);
    }
    return false;
}

json EnhancedBusinessHandler::parseRequestBody(const MyProtoMsg& msg) {
    try {
        // 从msg.body中解析JSON
        if (msg.body.count("data")) {
            return json::parse(msg.body["data"]);
        }
        return json::object();
    } catch (...) {
        return json::object();
    }
}

void EnhancedBusinessHandler::setResponse(const json& result, MyProtoMsg& response) {
    response.body["data"] = result.dump();
    response.body["success"] = result.contains("success") ? result["success"].get<bool>() : false;
    if (result.contains("message")) {
        response.body["message"] = result["message"].get<std::string>();
    }
}

int EnhancedBusinessHandler::getCurrentUserId(const MyProtoMsg& msg) {
    // 从消息中获取用户ID（假设在消息头中）
    if (msg.body.count("userId")) {
        return std::stoi(msg.body["userId"]);
    }
    return 0; // 未登录用户
}

void EnhancedBusinessHandler::handleLogin(int conn_id, MyProtoMsg& msg, MyProtoMsg& response) {
    json request = parseRequestBody(msg);
    std::string username = request.contains("username") ? request["username"] : "";
    std::string password = request.contains("password") ? request["password"] : "";
    
    json result = userService->login(username, password);
    setResponse(result, response);
}

void EnhancedBusinessHandler::handleGetUserPermissions(int conn_id, MyProtoMsg& msg, MyProtoMsg& response) {
    int userId = getCurrentUserId(msg);
    json result = userService->getUserPermissions(userId);
    setResponse(result, response);
}

void EnhancedBusinessHandler::handleGetStudentList(int conn_id, MyProtoMsg& msg, MyProtoMsg& response) {
    json request = parseRequestBody(msg);
    int page = request.contains("page") ? request["page"] : 1;
    int pageSize = request.contains("pageSize") ? request["pageSize"] : 10;
    int userId = getCurrentUserId(msg);
    
    json result = studentService->getStudentList(page, pageSize, userId);
    setResponse(result, response);
}

void EnhancedBusinessHandler::handleSearchStudent(int conn_id, MyProtoMsg& msg, MyProtoMsg& response) {
    json request = parseRequestBody(msg);
    std::string keyword = request.contains("keyword") ? request["keyword"] : "";
    int userId = getCurrentUserId(msg);
    
    json result = studentService->searchStudent(keyword, userId);
    setResponse(result, response);
}

void EnhancedBusinessHandler::handleGetStudentDetail(int conn_id, MyProtoMsg& msg, MyProtoMsg& response) {
    json request = parseRequestBody(msg);
    int studentId = request.contains("studentId") ? request["studentId"] : 0;
    int userId = getCurrentUserId(msg);
    
    json result = studentService->getStudentDetail(studentId, userId);
    setResponse(result, response);
}

void EnhancedBusinessHandler::handleAddStudent(int conn_id, MyProtoMsg& msg, MyProtoMsg& response) {
    json request = parseRequestBody(msg);
    int userId = getCurrentUserId(msg);
    
    json result = studentService->addStudent(request, userId);
    setResponse(result, response);
}

void EnhancedBusinessHandler::handleUpdateStudent(int conn_id, MyProtoMsg& msg, MyProtoMsg& response) {
    json request = parseRequestBody(msg);
    int userId = getCurrentUserId(msg);
    
    json result = studentService->updateStudent(request, userId);
    setResponse(result, response);
}

void EnhancedBusinessHandler::handleDeleteStudent(int conn_id, MyProtoMsg& msg, MyProtoMsg& response) {
    json request = parseRequestBody(msg);
    int studentId = request.contains("studentId") ? request["studentId"] : 0;
    int userId = getCurrentUserId(msg);
    
    json result = studentService->deleteStudent(studentId, userId);
    setResponse(result, response);
}

void EnhancedBusinessHandler::handleAddUser(int conn_id, MyProtoMsg& msg, MyProtoMsg& response) {
    json request = parseRequestBody(msg);
    
    json result = userService->addUser(request);
    setResponse(result, response);
}

void EnhancedBusinessHandler::handleUpdateUser(int conn_id, MyProtoMsg& msg, MyProtoMsg& response) {
    json request = parseRequestBody(msg);
    
    json result = userService->updateUser(request);
    setResponse(result, response);
}

void EnhancedBusinessHandler::handleDeleteUser(int conn_id, MyProtoMsg& msg, MyProtoMsg& response) {
    json request = parseRequestBody(msg);
    int userId = request.contains("userId") ? request["userId"] : 0;
    
    json result = userService->deleteUser(userId);
    setResponse(result, response);
}

void EnhancedBusinessHandler::handleGetAllUsers(int conn_id, MyProtoMsg& msg, MyProtoMsg& response) {
    json result = userService->getAllUsers();
    setResponse(result, response);
}