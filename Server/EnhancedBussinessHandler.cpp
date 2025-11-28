#include "EnhancedBusinessHandler.h"
#include "MyProto.h"
#include <iostream>
#include "json.hpp"
#include "asy.h" // 添加异步任务管理器头文件

using json = nlohmann::json;

EnhancedBusinessHandler* EnhancedBusinessHandler::instance = nullptr;

EnhancedBusinessHandler::EnhancedBusinessHandler() {
    userService = UserService::getInstance();
    studentService = StudentService::getInstance();
    businessHandler = nullptr;
    // 初始化异步任务管理器
    AsyncTaskManager::getInstance()->initialize(4);
}

EnhancedBusinessHandler* EnhancedBusinessHandler::getInstance() {
    if (!instance) {
        instance = new EnhancedBusinessHandler();
    }
    return instance;
}

EnhancedBusinessHandler::~EnhancedBusinessHandler() {
    if (instance) {
        AsyncTaskManager::getInstance()->shutdown();
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
        
        // 注册异步任务相关的处理器
        handler->registerHandler(3001, std::bind(&EnhancedBusinessHandler::handleSubmitLongTask, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        handler->registerHandler(3002, std::bind(&EnhancedBusinessHandler::handleGetTaskStatus, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        handler->registerHandler(3003, std::bind(&EnhancedBusinessHandler::handleCancelTask, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        handler->registerHandler(3004, std::bind(&EnhancedBusinessHandler::handleBatchProcessStudents, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        handler->registerHandler(3005, std::bind(&EnhancedBusinessHandler::handleBatchImportStudents, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    }
}

// 处理提交长时间任务的请求
void EnhancedBusinessHandler::handleSubmitLongTask(int conn_id, MyProtoMsg& msg, MyProtoMsg& response) {
    try {
        json request = parseRequestBody(msg);
        std::string operation_type = request.contains("operationType") ? request["operationType"] : "unknown";
        int userId = getCurrentUserId(msg);
        
        // 提交异步任务
        std::string task_id = AsyncTaskManager::getInstance()->submitTask(
            operation_type,
            userId,
            [operation_type](const std::string& task_id, std::function<void(int, const std::string&)> progress_callback) {
                // 模拟长时间操作
                for (int i = 1; i <= 10; ++i) {
                    std::this_thread::sleep_for(std::chrono::seconds(1)); // 模拟工作
                    int progress = i * 10;
                    std::string message = "Processing " + operation_type + "... Step " + std::to_string(i);
                    progress_callback(progress, message);
                }
                
                AsyncTaskManager::getInstance()->completeTask(task_id, operation_type + " completed successfully");
            }
        );
        
        // 返回任务ID给客户端
        json result;
        result["success"] = true;
        result["taskId"] = task_id;
        result["message"] = "Long operation submitted successfully";
        
        setResponse(result, response);
    } catch (const std::exception& e) {
        json result;
        result["success"] = false;
        result["message"] = std::string("Error submitting task: ") + e.what();
        setResponse(result, response);
    }
}

// 获取任务状态
void EnhancedBusinessHandler::handleGetTaskStatus(int conn_id, MyProtoMsg& msg, MyProtoMsg& response) {
    try {
        json request = parseRequestBody(msg);
        std::string task_id = request.contains("taskId") ? request["taskId"] : "";
        
        if (task_id.empty()) {
            json result;
            result["success"] = false;
            result["message"] = "Task ID is required";
            setResponse(result, response);
            return;
        }
        
        TaskInfo task_info = AsyncTaskManager::getInstance()->getTaskInfo(task_id);
        
        json result;
        result["success"] = true;
        result["taskId"] = task_info.task_id;
        result["status"] = static_cast<int>(task_info.status);
        result["statusText"] = [&task_info]() -> std::string {
            switch (task_info.status) {
                case TaskStatus::PENDING: return "PENDING";
                case TaskStatus::RUNNING: return "RUNNING";
                case TaskStatus::COMPLETED: return "COMPLETED";
                case TaskStatus::FAILED: return "FAILED";
                case TaskStatus::CANCELLED: return "CANCELLED";
                default: return "UNKNOWN";
            }
        }();
        result["progress"] = task_info.progress;
        result["message"] = task_info.error_message.empty() ? task_info.result : task_info.error_message;
        result["operationType"] = task_info.operation_type;
        result["elapsedTime"] = static_cast<long>(time(nullptr) - task_info.start_time);
        
        setResponse(result, response);
    } catch (const std::exception& e) {
        json result;
        result["success"] = false;
        result["message"] = std::string("Error getting task status: ") + e.what();
        setResponse(result, response);
    }
}

// 取消任务
void EnhancedBusinessHandler::handleCancelTask(int conn_id, MyProtoMsg& msg, MyProtoMsg& response) {
    try {
        json request = parseRequestBody(msg);
        std::string task_id = request.contains("taskId") ? request["taskId"] : "";
        
        if (task_id.empty()) {
            json result;
            result["success"] = false;
            result["message"] = "Task ID is required";
            setResponse(result, response);
            return;
        }
        
        bool cancelled = AsyncTaskManager::getInstance()->cancelTask(task_id);
        
        json result;
        result["success"] = cancelled;
        result["message"] = cancelled ? "Task cancelled successfully" : "Task cannot be cancelled (may already be running)";
        
        setResponse(result, response);
    } catch (const std::exception& e) {
        json result;
        result["success"] = false;
        result["message"] = std::string("Error cancelling task: ") + e.what();
        setResponse(result, response);
    }
}

// 批量处理学生信息（模拟长时间操作）
void EnhancedBusinessHandler::handleBatchProcessStudents(int conn_id, MyProtoMsg& msg, MyProtoMsg& response) {
    try {
        json request = parseRequestBody(msg);
        int userId = getCurrentUserId(msg);
        
        // 提交异步任务处理批量操作
        std::string task_id = AsyncTaskManager::getInstance()->submitTask(
            "batch_process_students",
            userId,
            [request, this](const std::string& task_id, std::function<void(int, const std::string&)> progress_callback) {
                // 模拟批量处理操作
                int total = request.contains("count") ? request["count"] : 100;
                
                progress_callback(0, "Starting batch process...");
                
                for (int i = 1; i <= total; ++i) {
                    // 模拟单条记录处理
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    
                    // 每处理10%更新一次进度
                    if (i % (total / 10) == 0 || i == total) {
                        int progress = (i * 100) / total;
                        std::string message = "Processed " + std::to_string(i) + " of " + std::to_string(total) + " records";
                        progress_callback(progress, message);
                    }
                }
                
                // 任务完成
                AsyncTaskManager::getInstance()->completeTask(task_id, "Batch processing completed: " + std::to_string(total) + " records processed");
            }
        );
        
        // 返回任务ID
        json result;
        result["success"] = true;
        result["taskId"] = task_id;
        result["message"] = "Batch processing started, use task ID to check progress";
        
        setResponse(result, response);
    } catch (const std::exception& e) {
        json result;
        result["success"] = false;
        result["message"] = std::string("Error starting batch process: ") + e.what();
        setResponse(result, response);
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
void EnhancedBusinessHandler::handleBatchImportStudents(int conn_id, MyProtoMsg& msg, MyProtoMsg& response) {
    try {
        json request = parseRequestBody(msg);
        int userId = getCurrentUserId(msg);
        
        // 提交异步任务
        std::string task_id = AsyncTaskManager::getInstance()->submitTask(
            "batch_import_students",
            userId,
            [request, this](const std::string& task_id, std::function<void(int, const std::string&)> progress_callback) {
                try {
                    // 从请求中获取学生数据
                    vector<json> students;
                    if (request.contains("students") && request["students"].is_array()) {
                        for (const auto& student : request["students"]) {
                            students.push_back(student);
                        }
                    }
                    
                    if (students.empty()) {
                        AsyncTaskManager::getInstance()->failTask(task_id, "没有可导入的学生数据");
                        return;
                    }
                    
                    // 更新初始进度
                    progress_callback(0, "开始批量导入学生数据...");
                    
                    // 调用数据库管理器进行批量导入
                    DatabaseManager* db_manager = DatabaseManager::getInstance();
                    bool success = db_manager->batchImportStudents(students, [task_id](int progress, const std::string& message) {
                        // 将进度更新传递给任务管理器
                        AsyncTaskManager::getInstance()->updateTaskProgress(task_id, progress, message);
                    });
                    
                    if (success) {
                        AsyncTaskManager::getInstance()->completeTask(task_id, "学生数据批量导入成功");
                    } else {
                        AsyncTaskManager::getInstance()->failTask(task_id, "学生数据批量导入失败");
                    }
                    
                } catch (const std::exception& e) {
                    AsyncTaskManager::getInstance()->failTask(task_id, "批量导入过程中发生异常: " + string(e.what()));
                }
            }
        );
        
        // 返回任务ID给客户端
        json result;
        result["success"] = true;
        result["taskId"] = task_id;
        result["message"] = "批量导入任务已提交，可通过任务ID查询进度";
        
        setResponse(result, response);
        
    } catch (const std::exception& e) {
        json result;
        result["success"] = false;
        result["message"] = "提交批量导入任务失败: " + string(e.what());
        setResponse(result, response);
    }
}