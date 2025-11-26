#include "business_handler.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>  // 添加这个头文件以使用 std::localtime

BusinessHandler::BusinessHandler() {
}

BusinessHandler::~BusinessHandler() {
    handlers_.clear();
}

void BusinessHandler::registerHandler(uint16_t server_id, MessageHandler handler) {
    handlers_[server_id] = handler;
    std::cout << "Registered handler for server_id: " << server_id << std::endl;
}

void BusinessHandler::removeHandler(uint16_t server_id) {
    handlers_.erase(server_id);
    std::cout << "Removed handler for server_id: " << server_id << std::endl;
}
// 新增函数：在控制台输出接收到的消息数据
void BusinessHandler::logMessage(int conn_id, const MyProtoMsg& msg) {
    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm = *std::localtime(&now_c);
    
    // 输出时间戳和连接信息
    std::cout << "[" << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S") << "] " 
              << "Connection ID: " << conn_id << ", "
              << "Server ID: " << msg.head.server << ", "
              << "Sequence: " << msg.head.sequence << ", "
              << "Data: ";
    
    // 输出消息体内容 - 直接处理 map 类型的 body
    std::cout << ", body size: " << msg.body.size() << " {";
    bool first = true;
    
    // 尝试直接访问 map 类型的 body
    try {
        // 由于我们看到 client_main.cpp 中使用了 msg.body["key"] = value 的语法
        // 假设 body 是一个 std::map 或类似的关联容器
        // 这里使用简单的方式尝试访问一些已知的键
        if (msg.body.size() > 0) {
            // 尝试访问几个常见的键来验证
            if (msg.body.count("type")) {
                std::cout << "\"type\": \"" << msg.body["type"] << "\"";
                first = false;
            }
            if (msg.body.count("message")) {
                if (!first) std::cout << ", ";
                std::cout << "\"message\": \"" << msg.body["message"] << "\"";
                first = false;
            }
            if (msg.body.count("timestamp")) {
                if (!first) std::cout << ", ";
                std::cout << "\"timestamp\": " << msg.body["timestamp"];
            }
            
            // 如果还有其他键，简单提示数量
            if (msg.body.size() > 3) {
                std::cout << ", ... +" << (msg.body.size() - 3) << " more fields";
            }
        }
    } catch (...) {
        std::cout << "<unable to access body content>";
    }
    
    std::cout << "}" << std::endl;
}
bool BusinessHandler::handleMessage(int conn_id, MyProtoMsg& msg) {
  // 在控制台输出接收到的消息
  logMessage(conn_id, msg);  
  auto it = handlers_.find(msg.head.server);
    if (it != handlers_.end()) {
        // 创建响应消息
        MyProtoMsg response;
        response.head.version = msg.head.version;
        response.head.server = msg.head.server;
        response.head.sequence = msg.head.sequence; // 使用相同的序列号
        response.head.type = 0; // 响应也是数据消息
        
        try {
            // 调用处理函数
            it->second(conn_id, msg, response);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Exception in message handler: " << e.what() << std::endl;
            return false;
        }
    } else {
        std::cerr << "No handler for server_id: " << msg.head.server << std::endl;
        return false;
    }
}

bool BusinessHandler::sendResponse(int conn_id, MyProtoMsg& response) {
    // 这个方法需要与ConnectionHandler集成
    // 实际的发送逻辑由ConnectionHandler实现
    std::cout << "Response prepared for conn_id: " << conn_id 
              << ", server_id: " << response.head.server << std::endl;
    return true;
}