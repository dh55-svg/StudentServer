#pragma once
#include "business_handler.h"
#include "UserService.h"
#include "studentService.h"

#include <functional>

class EnhancedBusinessHandler {
private:
    static EnhancedBusinessHandler* instance;
    UserService* userService;
    StudentService* studentService;
    BusinessHandler* businessHandler; // 保持对原有BusinessHandler的引用
    
    // 消息处理函数
    void handleLogin(int conn_id, MyProtoMsg& msg, MyProtoMsg& response);
    void handleGetUserPermissions(int conn_id, MyProtoMsg& msg, MyProtoMsg& response);
    void handleGetStudentList(int conn_id, MyProtoMsg& msg, MyProtoMsg& response);
    void handleSearchStudent(int conn_id, MyProtoMsg& msg, MyProtoMsg& response);
    void handleGetStudentDetail(int conn_id, MyProtoMsg& msg, MyProtoMsg& response);
    void handleAddStudent(int conn_id, MyProtoMsg& msg, MyProtoMsg& response);
    void handleUpdateStudent(int conn_id, MyProtoMsg& msg, MyProtoMsg& response);
    void handleDeleteStudent(int conn_id, MyProtoMsg& msg, MyProtoMsg& response);
    void handleAddUser(int conn_id, MyProtoMsg& msg, MyProtoMsg& response);
    void handleUpdateUser(int conn_id, MyProtoMsg& msg, MyProtoMsg& response);
    void handleDeleteUser(int conn_id, MyProtoMsg& msg, MyProtoMsg& response);
    void handleGetAllUsers(int conn_id, MyProtoMsg& msg, MyProtoMsg& response);
    void handleBatchImportStudents(int conn_id, MyProtoMsg& msg, MyProtoMsg& response)
    // 异步任务相关处理函数
    void handleSubmitLongTask(int conn_id, MyProtoMsg& msg, MyProtoMsg& response);
    void handleGetTaskStatus(int conn_id, MyProtoMsg& msg, MyProtoMsg& response);
    void handleCancelTask(int conn_id, MyProtoMsg& msg, MyProtoMsg& response);
    void handleBatchProcessStudents(int conn_id, MyProtoMsg& msg, MyProtoMsg& response);
    
    // 辅助方法
    nlohmann::json parseRequestBody(const MyProtoMsg& msg);
    void setResponse(const nlohmann::json& result, MyProtoMsg& response);
    int getCurrentUserId(const MyProtoMsg& msg);
    
    EnhancedBusinessHandler();

public:
    static EnhancedBusinessHandler* getInstance();
    ~EnhancedBusinessHandler();
     // 获取内部的BusinessHandler指针
    BusinessHandler* getBusinessHandler() {
        return businessHandler;
    }
    // 初始化并注册所有处理器
    void initialize(BusinessHandler* handler);
    
    // 消息转发处理
    bool forwardMessage(int conn_id, MyProtoMsg& msg);
};