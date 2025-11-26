#include <iostream>
#include <thread>
#include <chrono>
#include "Server.h"

// 主函数
int main(int argc, char* argv[]) {
    std::cout << "学生管理系统服务器启动..." << std::endl;
    
    // 创建服务器实例
    Server server;
    
    // 初始化服务器
    if (!server.initialize()) {
        std::cerr << "无法启动服务器：初始化失败" << std::endl;
        return -1;
    }
    
    // 启动服务器，监听8888端口
    const int SERVER_PORT = 8888;
    if (!server.start(SERVER_PORT)) {
        std::cerr << "服务器启动失败" << std::endl;
        return -1;
    }
    
    std::cout << "服务器启动成功，正在监听端口：" << SERVER_PORT << std::endl;
    std::cout << "按Ctrl+C停止服务器..." << std::endl;
    
    // 保持服务器运行
    while (server.isRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}