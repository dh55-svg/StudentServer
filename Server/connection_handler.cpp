#include "connection_handler.h"
#include "business_handler.h"
#include "reliable_msg_manager.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>

ConnectionHandler* ConnectionHandler::instance_ = nullptr;
ConnectionHandler::ConnectionHandler() : loop_(nullptr), server_(nullptr), 
    business_handler_(nullptr), reliable_msg_manager_(nullptr) {
    // 初始化协议解码器
    proto_decoder_.init();
}

ConnectionHandler::~ConnectionHandler() {
    stopServer();
    if (loop_) {
        hloop_stop(loop_);
        hloop_free(&loop_);
        loop_ = nullptr;
    }
}

bool ConnectionHandler::initialize(BusinessHandler* handler, ReliableMsgManager* msg_manager) {
    if (!handler || !msg_manager) {
        std::cerr << "Invalid handler or message manager" << std::endl;
        return false;
    }
    
    business_handler_ = handler;
    reliable_msg_manager_ = msg_manager;
    
    // 创建事件循环
    loop_ = hloop_new();
    if (!loop_) {
        std::cerr << "Failed to create event loop" << std::endl;
        return false;
    }
    
    // 设置事件循环到可靠消息管理器
    reliable_msg_manager_->setEventLoop(loop_);
    
    // 设置重传回调函数
    reliable_msg_manager_->setRetransmitCallback(
        [this](int conn_id, const MyProtoMsg& msg) {
            this->onRetransmitMessage(conn_id, msg);
        }
    );
    
    reliable_msg_manager_->startTimeoutCheck();
    setInstance(this); // 设置全局实例
    
    return true;
}

void ConnectionHandler::onRetransmitMessage(int conn_id, const MyProtoMsg& msg) {
    // 复制消息对象，准备发送
    MyProtoMsg retransmit_msg = msg;
    
    // 找到对应的连接
    auto it = clients_.find(conn_id);
    if (it != clients_.end()) {
        std::cout << "Performing actual retransmit, conn_id: " << conn_id 
                  << ", sequence: " << msg.head.sequence << std::endl;
        
        // 发送重传消息
        sendMessage(it->second.io, retransmit_msg);
    } else {
        std::cerr << "Connection not found for retransmit, conn_id: " << conn_id << std::endl;
    }
}

bool ConnectionHandler::startServer(int port) {
    if (!loop_) {
        std::cerr << "Event loop not initialized" << std::endl;
        return false;
    }
    
    // 创建TCP服务器
    server_ = hloop_create_tcp_server(loop_, "0.0.0.0", port, ConnectionHandler::onConnection);
    if (!server_) {
        std::cerr << "Failed to create TCP server on port " << port << std::endl;
        return false;
    }
    
    // 设置用户数据，方便回调函数访问
    hio_set_context(server_, this);
    
    std::cout << "Server started on port " << port << std::endl;
    
    // 启动事件循环
    hloop_run(loop_);
    
    return true;
}

void ConnectionHandler::stopServer() {
    if (server_) {
        hio_close(server_);
        server_ = nullptr;
    }
    
    // 关闭所有客户端连接
    for (auto& pair : clients_) {
        hio_close(pair.second.io);
    }
    clients_.clear();
}

hio_t* ConnectionHandler::connectToServer(const char* host, int port) {
    if (!loop_) {
        std::cerr << "Event loop not initialized" << std::endl;
        return nullptr;
    }
    
    hio_t* io = hloop_create_tcp_client(loop_, host, port, ConnectionHandler::onConnection, ConnectionHandler::onClose);
    if (!io) {
        std::cerr << "Failed to connect to server " << host << ":" << port << std::endl;
        return nullptr;
    }
    
    // 设置用户数据
    hio_set_context(io, this);
    
    return io;
}

bool ConnectionHandler::sendMessage(hio_t* conn, MyProtoMsg& msg) {
    if (!conn || !reliable_msg_manager_) {
        return false;
    }
    
    int conn_id = getConnectionId(conn);
    
    // 生成序列号
    msg.head.sequence = reliable_msg_manager_->generateSequence(conn_id);
    
    // 保存待发送消息
    reliable_msg_manager_->savePendingMessage(conn_id, msg);
    
    // 编码消息
    uint32_t len = 0;
    uint8_t* data = proto_encoder_.encode(&msg, len);
    if (!data) {
        return false;
    }
    
    // 发送数据
    int n = hio_write(conn, data, len);
    delete[] data;
    
    return n == len;
}

bool ConnectionHandler::sendMessage(int conn_id, MyProtoMsg& msg) {
    auto it = clients_.find(conn_id);
    if (it == clients_.end()) {
        return false;
    }
    return sendMessage(it->second.io, msg);
}

void ConnectionHandler::broadcastMessage(MyProtoMsg& msg) {
    for (auto& pair : clients_) {
        MyProtoMsg cloned_msg = msg;
        sendMessage(pair.second.io, cloned_msg);
    }
}

void ConnectionHandler::onConnection(hio_t* io) {
    std::cout << "[DEBUG] onConnection called, fd: " << hio_fd(io) << std::endl;
    
    // 获取全局实例
    ConnectionHandler* handler = getInstance();
    if (!handler) {
        std::cerr << "[ERROR] No ConnectionHandler instance available" << std::endl;
        return;
    }
    // 设置连接的回调函数
    hio_setcb_close(io, ConnectionHandler::onClose);
    hio_setcb_read(io, ConnectionHandler::onMessage);
    // 设置新连接的上下文
    hio_set_context(io, handler);
    // 开始读取数据
    hio_read(io);
    
    int conn_id = handler->getConnectionId(io);
    
    // 创建连接信息
    ConnectionInfo conn_info;
    conn_info.io = io;
    conn_info.last_heartbeat_time = time(nullptr);
    
    // 创建超时定时器
    conn_info.timeout_timer = htimer_add(handler->loop_, ConnectionHandler::onHeartbeatTimeout, 
                                         handler->heartbeat_timeout_, 1);
    // 设置timer->data为handler，priv_data为conn_id
   hevent_set_userdata((hevent_t*)conn_info.timeout_timer, handler);
   ((hevent_t*)(conn_info.timeout_timer))->privdata = (void*)(intptr_t)conn_id;
    
    handler->clients_[conn_id] = conn_info;
    
    // 设置心跳
    hio_set_heartbeat(io, handler->heartbeat_interval_, ConnectionHandler::sendHeartbeat);
    
    std::cout << "[DEBUG] Client connected, id: " << conn_id << ", heartbeat interval: " 
              << handler->heartbeat_interval_ << "ms" << std::endl;
    std::cout.flush(); // 强制刷新输出
}

void ConnectionHandler::onClose(hio_t* io) {
    ConnectionHandler* handler = (ConnectionHandler*)hio_context(io);
    if (!handler) return;
    
    int conn_id = handler->getConnectionId(io);
    auto it = handler->clients_.find(conn_id);
    if (it != handler->clients_.end()) {
        // 取消超时定时器
        if (it->second.timeout_timer) {
            htimer_del(it->second.timeout_timer);
        }
        
        handler->clients_.erase(it);
    }
    
    if (handler->reliable_msg_manager_) {
        handler->reliable_msg_manager_->removeConnection(conn_id);
    }
    std::cout << "Client disconnected, id: " << conn_id << std::endl;
}

void ConnectionHandler::onMessage(hio_t* io, void* buf, int readbytes) {
    std::cout << "[DEBUG] onMessage called, readbytes: " << readbytes << std::endl;
    
    // 输出原始数据的十六进制转储
    std::cout << "[DEBUG] Raw data hex dump:" << std::endl;
    uint8_t* data_ptr = (uint8_t*)buf;
    for (int i = 0; i < readbytes; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data_ptr[i] << " ";
        if ((i + 1) % 16 == 0) std::cout << std::endl;
    }
    std::cout << std::dec << std::endl;
    
    ConnectionHandler* handler = (ConnectionHandler*)hio_context(io);
    if (!handler) {
        std::cerr << "[ERROR] handler is null in onMessage!" << std::endl;
        return;
    }
    
    try {
        // 解析协议消息
        std::cout << "[DEBUG] Calling proto_decoder_.parser()" << std::endl;
        bool parse_result = handler->proto_decoder_.parser(buf, readbytes);
        std::cout << "[DEBUG] parser() returned: " << (parse_result ? "true" : "false") << std::endl;
        
        if (parse_result) {
            std::cout << "[DEBUG] Checking message queue, empty: " << (handler->proto_decoder_.empty() ? "true" : "false") << std::endl;
            // 处理所有解析出的消息
            while (!handler->proto_decoder_.empty()) {
                std::cout << "[DEBUG] Found message in queue, calling processMessage" << std::endl;
                std::shared_ptr<MyProtoMsg> msg = handler->proto_decoder_.front();
                if (!msg) {
                    std::cerr << "[ERROR] Message pointer is null!" << std::endl;
                    handler->proto_decoder_.pop();
                    continue;
                }
                handler->processMessage(io, msg);
                handler->proto_decoder_.pop();
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in onMessage: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception in onMessage" << std::endl;
    }
    
    // 继续读取数据
    std::cout << "[DEBUG] Calling hio_read to continue reading" << std::endl;
    hio_read(io);
    std::cout.flush(); // 强制刷新输出
}

void ConnectionHandler::processMessage(hio_t* io, std::shared_ptr<MyProtoMsg> msg) {
    try {
        int conn_id = getConnectionId(io);
        
        // 处理心跳请求消息
        if (msg->head.type == MY_PROTO_TYPE_HEARTBEAT) {
            std::cout << "[DEBUG] Received heartbeat request, conn_id: " << conn_id << std::endl;
            
            // 更新最后心跳时间
            auto it = clients_.find(conn_id);
            if (it != clients_.end()) {
                it->second.last_heartbeat_time = time(nullptr);
            }
            
            // 发送心跳响应
            MyProtoMsg heartbeat_ack;
            heartbeat_ack.head.version = msg->head.version;
            heartbeat_ack.head.server = msg->head.server;
            heartbeat_ack.head.sequence = msg->head.sequence;
            heartbeat_ack.head.type = MY_PROTO_TYPE_HEARTBEAT_ACK;
            heartbeat_ack.body = {"timestamp", time(nullptr)};
            
            sendMessage(io, heartbeat_ack);
            return;
        }
        
        // 处理心跳响应消息
        if (msg->head.type == MY_PROTO_TYPE_HEARTBEAT_ACK) {
            std::cout << "[DEBUG] Received heartbeat response, conn_id: " << conn_id << std::endl;
            
            // 更新最后心跳时间
            auto it = clients_.find(conn_id);
            if (it != clients_.end()) {
                it->second.last_heartbeat_time = time(nullptr);
            }
            return;
        }
        
        // 处理确认消息
        if (msg->head.type == MY_PROTO_TYPE_ACK) {
            std::cout << "[DEBUG] Processing confirmation message" << std::endl;
            // 处理消息确认
            if (reliable_msg_manager_) {
                reliable_msg_manager_->processConfirmation(conn_id, msg->head.sequence);
                std::cout << "[DEBUG] Confirmation processed for sequence: " << msg->head.sequence << std::endl;
            } else {
                std::cerr << "[WARN] reliable_msg_manager_ is null" << std::endl;
            }
            return;
        }
        
        // 处理数据消息
        if (business_handler_) {
            std::cout << "[DEBUG] Processing data message, calling business_handler_->handleMessage" << std::endl;
            // 创建确认消息
            MyProtoMsg ack_msg;
            ack_msg.head.version = msg->head.version;
            ack_msg.head.server = msg->head.server;
            ack_msg.head.sequence = msg->head.sequence;
            ack_msg.head.type = 1; // 确认消息类型
            ack_msg.body = json::object();
            
            // 发送确认消息
            uint32_t len = 0;
            uint8_t* ack_data = proto_encoder_.encode(&ack_msg, len);
            if (ack_data) {
                hio_write(io, ack_data, len);
                std::cout << "[DEBUG] Sent acknowledgment message, len: " << len << std::endl;
                delete[] ack_data;
            } else {
                std::cerr << "[ERROR] Failed to encode acknowledgment message" << std::endl;
            }
            
            // 调用业务处理器处理消息
            business_handler_->handleMessage(conn_id, *msg);
            std::cout << "[DEBUG] Business handler called successfully" << std::endl;
        } else {
            std::cerr << "[ERROR] business_handler_ is null" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in processMessage: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception in processMessage" << std::endl;
    }
    std::cout.flush(); // 强制刷新输出
}

int ConnectionHandler::getConnectionId(hio_t* io) {
    // 使用连接的ID作为唯一标识
    return hio_id(io);
}

// 发送心跳消息的回调函数
void ConnectionHandler::sendHeartbeat(hio_t* io) {
    ConnectionHandler* handler = (ConnectionHandler*)hio_context(io);
    if (!handler) return;
    
    int conn_id = handler->getConnectionId(io);
    
    // 创建心跳消息
    MyProtoMsg heartbeat_msg;
    heartbeat_msg.head.version = 1;
    heartbeat_msg.head.server = 0; // 心跳消息不需要特定服务号
    heartbeat_msg.head.type = MY_PROTO_TYPE_HEARTBEAT;
    heartbeat_msg.body = {"timestamp", time(nullptr)}; // 包含时间戳
    
    std::cout << "[DEBUG] Sending heartbeat, conn_id: " << conn_id << std::endl;
    handler->sendMessage(io, heartbeat_msg);
}

// 心跳超时检查的回调函数
void ConnectionHandler::onHeartbeatTimeout(htimer_t* timer) {
    // 使用hevent_userdata API代替直接访问timer->data
    ConnectionHandler* handler = (ConnectionHandler*)hevent_userdata((hevent_t*)timer);
    if (!handler) return;
    
    // 使用hevent_privdata API代替直接访问timer->priv_data
    int conn_id = (int)(intptr_t)((hevent_t*)timer)->privdata;
    auto it = handler->clients_.find(conn_id);
    if (it == handler->clients_.end()) return;
    
    time_t current_time = time(nullptr);
    time_t last_heartbeat = it->second.last_heartbeat_time;
    
    // 检查是否超时
    if (current_time - last_heartbeat > handler->heartbeat_timeout_ / 1000) {
        std::cout << "[WARN] Heartbeat timeout, closing connection, conn_id: " << conn_id << std::endl;
        hio_close(it->second.io);
    } else {
        htimer_reset(timer, handler->heartbeat_timeout_);
    }
}
void ConnectionHandler::setHeartbeatConfig(int interval_ms, int timeout_ms) {
    heartbeat_interval_ = interval_ms;
    heartbeat_timeout_ = timeout_ms;
}