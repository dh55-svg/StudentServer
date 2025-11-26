#ifndef __CONNECTION_HANDLER_H__
#define __CONNECTION_HANDLER_H__

#include <hv/hv.h>
#include <hv/hloop.h>

#include "myproto.h"
#include <unordered_map>

// 前向声明
class BusinessHandler;
class ReliableMsgManager;

// 连接处理器类，负责网络连接管理
class ConnectionHandler {
private:
    hloop_t* loop_;               // libhv事件循环
    hio_t* server_;               // 服务器连接
    MyProtoEncode proto_encoder_; // 协议编码器
    MyProtoDecode proto_decoder_; // 协议解码器
    BusinessHandler* business_handler_; // 业务处理器指针
    ReliableMsgManager* reliable_msg_manager_; // 可靠消息管理器
    static ConnectionHandler* instance_; // 静态实例指针
    uint32_t  heartbeat_interval_; // 心跳间隔（毫秒）
    uint32_t  heartbeat_timeout_;  // 心跳超时时间（毫秒）
    
    // 连接信息结构体，用于存储连接的心跳相关信息
    struct ConnectionInfo {
        hio_t* io;                 // 连接对象
        time_t last_heartbeat_time; // 最后一次收到心跳的时间
        htimer_t* timeout_timer;    // 超时定时器
    };
    
    // 使用新的结构体来存储连接信息
    std::unordered_map<int, ConnectionInfo> clients_;  // 客户端连接映射
    
    // 发送心跳消息的回调函数
    static void sendHeartbeat(hio_t* io);
    
    // 心跳超时检查的回调函数
    static void onHeartbeatTimeout(htimer_t* timer);
private:
    // 连接建立回调
    static void onConnection(hio_t* io);
    
    // 连接关闭回调
    static void onClose(hio_t* io);
    
    // 数据接收回调
    static void onMessage(hio_t* io, void* buf, int readbytes);
    
    // 处理接收到的协议消息
    void processMessage(hio_t* io, std::shared_ptr<MyProtoMsg> msg);
    
    // 获取连接ID
    int getConnectionId(hio_t* io);
    
    // 重传消息回调
    void onRetransmitMessage(int conn_id, const MyProtoMsg& msg);
public:
    ConnectionHandler();
    ~ConnectionHandler();

    // 初始化连接处理器
    bool initialize(BusinessHandler* handler, ReliableMsgManager* msg_manager);
    
    // 启动服务器
    bool startServer(int port);
    
    // 停止服务器
    void stopServer();
    
    // 连接到服务器
    hio_t* connectToServer(const char* host, int port);
    
    // 发送消息
    bool sendMessage(hio_t* conn, MyProtoMsg& msg);
    
    // 发送消息(使用连接ID)
    bool sendMessage(int conn_id, MyProtoMsg& msg);
    
    // 广播消息给所有客户端
    void broadcastMessage(MyProtoMsg& msg);
    
    // 获取事件循环
    hloop_t* getEventLoop() { return loop_; }
    static ConnectionHandler* getInstance() { return instance_; }
    static void setInstance(ConnectionHandler* instance) { instance_ = instance; }
    
    // 设置心跳参数
    void setHeartbeatConfig(int interval_ms, int timeout_ms);
};

#endif // __CONNECTION_HANDLER_H__