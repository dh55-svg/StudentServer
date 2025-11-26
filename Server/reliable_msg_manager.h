#ifndef __RELIABLE_MSG_MANAGER_H__
#define __RELIABLE_MSG_MANAGER_H__

#include <map>
#include <chrono>
#include <memory>
#include <functional>
#include "myproto.h"
#include <hv/hloop.h>

// 消息状态枚举
enum class MessageStatus {
    PENDING_ACK,   // 等待确认
    ACKED,         // 已确认
    TIMEOUT,       // 超时
    RETRANSMITTED  // 已重传
};

// 待确认消息结构体
struct PendingMessage {
    MyProtoMsg msg;                      // 消息内容
    MessageStatus status;                // 消息状态
    int retransmit_count;                // 重传次数
    std::chrono::steady_clock::time_point send_time;  // 发送时间
};

// 连接状态结构体
struct ConnectionStatus {
    uint32_t next_sequence;              // 下一个要发送的序列号
    std::map<uint32_t, PendingMessage> pending_messages;  // 待确认消息
};

// 重传回调函数类型
typedef std::function<void(int conn_id, const MyProtoMsg& msg)> RetransmitCallback;

// 可靠消息管理器类
class ReliableMsgManager {
private:
    std::map<int, ConnectionStatus> connections_;  // 连接ID到连接状态的映射
    int max_retransmits_;                         // 最大重传次数
    int retransmit_interval_;                     // 重传间隔（毫秒）
    hloop_t* loop_;                               // 事件循环指针
    htimer_t* timeout_timer_;                     // 超时检测定时器
    RetransmitCallback retransmit_callback_;      // 重传回调函数

public:
    ReliableMsgManager(int max_retransmits = 3, int retransmit_interval = 1000);
    ~ReliableMsgManager();

    // 设置事件循环
    void setEventLoop(hloop_t* loop);
    
    // 设置重传回调函数
    void setRetransmitCallback(RetransmitCallback callback);
    
    // 生成序列号
    uint32_t generateSequence(int conn_id);
    
    // 保存待发送消息
    void savePendingMessage(int conn_id, const MyProtoMsg& msg);
    
    // 处理消息确认
    void processConfirmation(int conn_id, uint32_t sequence);
    
    // 移除连接
    void removeConnection(int conn_id);
    
    // 启动超时检测
    void startTimeoutCheck();
    
    // 停止超时检测
    void stopTimeoutCheck();

private:
    // 检查超时消息
    void checkTimeouts();
    
    // 重传消息
    void retransmitMessage(int conn_id, uint32_t sequence);
    
    // 定时器回调函数
    static void timeoutCallback(htimer_t* timer);
};

#endif // __RELIABLE_MSG_MANAGER_H__