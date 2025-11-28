#include "asy.h"
#include <iostream>
#include <random>
#include <sstream>
#include <ctime>

// 静态成员初始化
AsyncTaskManager* AsyncTaskManager::instance_ = nullptr;
std::mutex AsyncTaskManager::instance_mutex_;

AsyncTaskManager::AsyncTaskManager() : stop_(false) {
}

AsyncTaskManager* AsyncTaskManager::getInstance() {
    if (!instance_) {
        std::lock_guard<std::mutex> lock(instance_mutex_);
        if (!instance_) {
            instance_ = new AsyncTaskManager();
        }
    }
    return instance_;
}

void AsyncTaskManager::initialize(int num_threads) {
    for (int i = 0; i < num_threads; ++i) {
        worker_threads_.emplace_back(&AsyncTaskManager::workerThread, this);
    }
    std::cout << "AsyncTaskManager initialized with " << num_threads << " worker threads" << std::endl;
}

void AsyncTaskManager::shutdown() {
    stop_ = true;
    condition_.notify_all();
    
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    worker_threads_.clear();
    std::cout << "AsyncTaskManager shutdown" << std::endl;
}

void AsyncTaskManager::workerThread() {
    while (!stop_) {
        std::function<void()> task;
        bool got_task = false;
        
        {   
            std::unique_lock<std::mutex> lock(queue_mutex_);
            condition_.wait(lock, [this] { return stop_ || !task_queue_.empty(); });
            
            if (!stop_ && !task_queue_.empty()) {
                task = std::move(task_queue_.front());
                task_queue_.pop();
                got_task = true;
            }
        }
        
        if (got_task) {
            try {
                task();
            } catch (const std::exception& e) {
                std::cerr << "Exception in async task: " << e.what() << std::endl;
            }
        }
    }
}

std::string AsyncTaskManager::generateTaskId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> distrib(0, 9);
    
    std::stringstream ss;
    time_t now = time(0);
    ss << now;
    for (int i = 0; i < 8; ++i) {
        ss << distrib(gen);
    }
    return ss.str();
}

std::string AsyncTaskManager::submitTask(
    const std::string& operation_type,
    int user_id,
    std::function<void(const std::string&, std::function<void(int, const std::string&)>)> task_func) {
    
    std::string task_id = generateTaskId();
    
    // 创建任务信息
    TaskInfo task_info;
    task_info.task_id = task_id;
    task_info.operation_type = operation_type;
    task_info.status = TaskStatus::PENDING;
    task_info.progress = 0;
    task_info.user_id = user_id;
    task_info.start_time = time(nullptr);
    
    // 添加到任务映射
    {   
        std::lock_guard<std::mutex> lock(tasks_mutex_);
        tasks_[task_id] = task_info;
    }
    
    
    // 将任务加入队列
    {   
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_queue_.emplace([this, task_id, task_func]() {
            // 更新任务状态为运行中
            {   
                std::lock_guard<std::mutex> lock(tasks_mutex_);
                if (tasks_.find(task_id) != tasks_.end()) {
                    tasks_[task_id].status = TaskStatus::RUNNING;
                }
            }
            
            // 执行任务
            try {
                task_func(task_id, [this, task_id](int progress, const std::string& message = "") {
                    updateTaskProgress(task_id, progress, message);
                });
            } catch (const std::exception& e) {
                failTask(task_id, std::string("Task execution failed: ") + e.what());
            }
        });
    }
    
    condition_.notify_one();
    std::cout << "Submitted task: " << task_id << " (" << operation_type << ") for user " << user_id << std::endl;
    
    return task_id;
}

void AsyncTaskManager::updateTaskProgress(const std::string& task_id, int progress, const std::string& message) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    auto it = tasks_.find(task_id);
    if (it != tasks_.end()) {
        it->second.progress = std::max(0, std::min(100, progress));
        if (!message.empty()) {
            it->second.result = message; // 使用result字段存储进度消息
        }
        std::cout << "Task " << task_id << " progress: " << progress << "% - " << message << std::endl;
    }
}

void AsyncTaskManager::completeTask(const std::string& task_id, const std::string& result) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    auto it = tasks_.find(task_id);
    if (it != tasks_.end()) {
        it->second.status = TaskStatus::COMPLETED;
        it->second.progress = 100;
        it->second.result = result;
        std::cout << "Task " << task_id << " completed successfully" << std::endl;
    }
}

void AsyncTaskManager::failTask(const std::string& task_id, const std::string& error_message) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    auto it = tasks_.find(task_id);
    if (it != tasks_.end()) {
        it->second.status = TaskStatus::FAILED;
        it->second.error_message = error_message;
        std::cout << "Task " << task_id << " failed: " << error_message << std::endl;
    }
}

TaskInfo AsyncTaskManager::getTaskInfo(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    auto it = tasks_.find(task_id);
    if (it != tasks_.end()) {
        return it->second;
    }
    
    // 返回空任务信息
    TaskInfo empty;
    empty.task_id = task_id;
    empty.status = TaskStatus::FAILED;
    empty.error_message = "Task not found";
    return empty;
}

bool AsyncTaskManager::cancelTask(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    auto it = tasks_.find(task_id);
    if (it != tasks_.end() && it->second.status == TaskStatus::PENDING) {
        it->second.status = TaskStatus::CANCELLED;
        it->second.error_message = "Task cancelled by user";
        std::cout << "Task " << task_id << " cancelled" << std::endl;
        return true;
    }
    return false;
}