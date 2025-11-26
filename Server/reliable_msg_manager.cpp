#include "reliable_msg_manager.h"
#include <iostream>

ReliableMsgManager::ReliableMsgManager(int max_retransmits, int retransmit_interval)
    : max_retransmits_(max_retransmits), retransmit_interval_(retransmit_interval),
      loop_(nullptr), timeout_timer_(nullptr), retransmit_callback_(nullptr) {
}

ReliableMsgManager::~ReliableMsgManager() {
    stopTimeoutCheck();
}

void ReliableMsgManager::setEventLoop(hloop_t* loop) {
    loop_ = loop;
}

void ReliableMsgManager::setRetransmitCallback(RetransmitCallback callback) {
    retransmit_callback_ = callback;
}

uint32_t ReliableMsgManager::generateSequence(int conn_id) {
    // 为每个连接维护独立的序列号
    auto& conn = connections_[conn_id];
    uint32_t sequence = conn.next_sequence;
    conn.next_sequence++;
    return sequence;
}

void ReliableMsgManager::savePendingMessage(int conn_id, const MyProtoMsg& msg) {
    auto& conn = connections_[conn_id];
    PendingMessage pending_msg;
    pending_msg.msg = msg;
    pending_msg.status = MessageStatus::PENDING_ACK;
    pending_msg.retransmit_count = 0;
    pending_msg.send_time = std::chrono::steady_clock::now();
    
    conn.pending_messages[msg.head.sequence] = pending_msg;
    
    std::cout << "Saved pending message, conn_id: " << conn_id 
              << ", sequence: " << msg.head.sequence << std::endl;
}

void ReliableMsgManager::processConfirmation(int conn_id, uint32_t sequence) {
    auto it_conn = connections_.find(conn_id);
    if (it_conn == connections_.end()) {
        return;
    }
    
    auto& pending_messages = it_conn->second.pending_messages;
    auto it_msg = pending_messages.find(sequence);
    if (it_msg != pending_messages.end()) {
        it_msg->second.status = MessageStatus::ACKED;
        std::cout << "Message confirmed, conn_id: " << conn_id 
                  << ", sequence: " << sequence << std::endl;
        
        // 移除已确认的消息
        pending_messages.erase(it_msg);
    }
}

void ReliableMsgManager::removeConnection(int conn_id) {
    connections_.erase(conn_id);
    std::cout << "Connection removed, conn_id: " << conn_id << std::endl;
}

void ReliableMsgManager::startTimeoutCheck() {
    if (!loop_) {
        std::cerr << "Event loop not set" << std::endl;
        return;
    }
    
    if (!timeout_timer_) {
        timeout_timer_ = htimer_add(loop_, timeoutCallback, retransmit_interval_, retransmit_interval_);
        if (timeout_timer_) {
            hevent_set_userdata(timeout_timer_, this);
            std::cout << "Timeout check started" << std::endl;
        }
    }
}

void ReliableMsgManager::stopTimeoutCheck() {
    if (timeout_timer_) {
        htimer_del(timeout_timer_);
        timeout_timer_ = nullptr;
        std::cout << "Timeout check stopped" << std::endl;
    }
}

void ReliableMsgManager::checkTimeouts() {
    auto now = std::chrono::steady_clock::now();
    
    for (auto& conn_pair : connections_) {
        int conn_id = conn_pair.first;
        auto& pending_messages = conn_pair.second.pending_messages;
        
        // 使用迭代器安全删除
        for (auto it = pending_messages.begin(); it != pending_messages.end();) {
            auto& pending_msg = it->second;
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - pending_msg.send_time).count();
            
            if (pending_msg.status == MessageStatus::PENDING_ACK && elapsed > retransmit_interval_) {
                if (pending_msg.retransmit_count < max_retransmits_) {
                    // 执行重传操作
                    retransmitMessage(conn_id, it->first);
                    ++it; // 继续下一个消息
                } else {
                    // 超过最大重传次数
                    std::cout << "Message max retransmits reached, conn_id: " << conn_id 
                              << ", sequence: " << it->first << std::endl;
                    pending_msg.status = MessageStatus::TIMEOUT;
                    
                    // 删除超时消息
                    it = pending_messages.erase(it);
                }
            } else {
                ++it; // 继续下一个消息
            }
        }
    }
}

void ReliableMsgManager::retransmitMessage(int conn_id, uint32_t sequence) {
    auto it_conn = connections_.find(conn_id);
    if (it_conn == connections_.end()) {
        return;
    }
    
    auto& pending_messages = it_conn->second.pending_messages;
    auto it_msg = pending_messages.find(sequence);
    if (it_msg != pending_messages.end()) {
        PendingMessage& pending_msg = it_msg->second;
        
        // 更新消息状态和重传计数
        pending_msg.status = MessageStatus::RETRANSMITTED;
        pending_msg.retransmit_count++;
        pending_msg.send_time = std::chrono::steady_clock::now();
        
        std::cout << "Retransmitting message, conn_id: " << conn_id 
                  << ", sequence: " << sequence 
                  << ", retransmit_count: " << pending_msg.retransmit_count << std::endl;
        
        // 调用回调函数进行实际重传
        if (retransmit_callback_) {
            retransmit_callback_(conn_id, pending_msg.msg);
        }
        
        // 重传后状态变回等待确认
        pending_msg.status = MessageStatus::PENDING_ACK;
    }
}

void ReliableMsgManager::timeoutCallback(htimer_t* timer) {
    ReliableMsgManager* manager = (ReliableMsgManager*)hevent_userdata(timer);
    if (manager) {
        manager->checkTimeouts();
    }
}