// AsyncTaskManager.h - 新增文件
#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

enum class TaskStatus {
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED,
    CANCELLED
};

struct TaskInfo {
    std::string task_id;
    std::string operation_type;
    TaskStatus status;
    int progress;
    std::string result;
    std::string error_message;
    int user_id;
    time_t start_time;
};

class AsyncTaskManager {
private:
    AsyncTaskManager();
    static AsyncTaskManager* instance_;
    static std::mutex instance_mutex_;
    
    std::unordered_map<std::string, TaskInfo> tasks_;
    std::queue<std::function<void()>> task_queue_;
    std::vector<std::thread> worker_threads_;
    std::mutex tasks_mutex_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
    
    void workerThread();
    std::string generateTaskId();
    
public:
    static AsyncTaskManager* getInstance();
    
    void initialize(int num_threads = 4);
    void shutdown();
    
    // 提交任务并返回任务ID
    std::string submitTask(
        const std::string& operation_type,
        int user_id,
        std::function<void(const std::string&, std::function<void(int, const std::string&)>)> task_func
    );
    
    // 更新任务进度
    void updateTaskProgress(const std::string& task_id, int progress, const std::string& message = "");
    
    // 完成任务
    void completeTask(const std::string& task_id, const std::string& result = "");
    
    // 任务失败
    void failTask(const std::string& task_id, const std::string& error_message);
    
    // 获取任务状态
    TaskInfo getTaskInfo(const std::string& task_id);
    
    // 取消任务
    bool cancelTask(const std::string& task_id);
};