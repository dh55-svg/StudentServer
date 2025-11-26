#ifndef __BUSINESS_HANDLER_H__
#define __BUSINESS_HANDLER_H__

#include <unordered_map>
#include <functional>
#include <iostream>
#include <chrono>
#include <iomanip>
#include "Myproto.h"

// 消息处理函数类型定义
typedef std::function<void(int conn_id, MyProtoMsg& msg, MyProtoMsg& response)> MessageHandler;

// 业务处理器类
class BusinessHandler {
private:
    // 服务号到处理函数的映射
    std::unordered_map<uint16_t, MessageHandler> handlers_;
    void logMessage(int conn_id, const MyProtoMsg& msg);
public:
    BusinessHandler();
    ~BusinessHandler();

    // 注册消息处理函数
    void registerHandler(uint16_t server_id, MessageHandler handler);
    
    // 移除消息处理函数
    void removeHandler(uint16_t server_id);
    
    // 处理消息
    bool handleMessage(int conn_id, MyProtoMsg& msg);
    
    // 发送响应（通常由ConnectionHandler调用）
    bool sendResponse(int conn_id, MyProtoMsg& response);
    
};

#endif // __BUSINESS_HANDLER_H__