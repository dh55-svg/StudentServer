#include "UserDao.h"
#include "DatabaseManager.h"
#include <mysql/mysql.h>

UserDAO* UserDAO::instance = nullptr;

UserDAO::UserDAO() {
}

UserDAO* UserDAO::getInstance() {
    if (!instance) {
        instance = new UserDAO();
    }
    return instance;
}

UserDAO::~UserDAO() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}

UserModel* UserDAO::authenticateUser(const std::string& username, const std::string& password) {
    // 调用DatabaseManager的认证方法
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    json result=dbManager->authenticateUser(username, password);
    // 检查认证是否成功
    if (result.contains("success") && result["success"] == true && result.contains("user")) {
        // 使用fromJson静态方法转换并返回新创建的UserModel对象指针
        UserModel* user = new UserModel(UserModel::fromJson(result["user"]));
        return user;
    }
    // 认证失败返回nullptr
    return nullptr;
}

std::vector<std::string> UserDAO::getUserPermissions(int userId) {
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    UserModel *user=getUserById(userId);
    if(!user){
        return;
    }
    std::string username = user->getUsername();
    delete user;  // 释放UserModel对象
    // 调用DatabaseManager的getUserPermissions方法获取权限json
    json permissionsJson = dbManager->getUserPermissions(username);
    // 转换json为std::vector<std::string>
    std::vector<std::string> permissions;
    
    // 假设json中包含一个名为"permissions"的数组字段
    if (permissionsJson.contains("permissions") && permissionsJson["permissions"].is_array()) {
        for (const auto& perm : permissionsJson["permissions"]) {
            if (perm.is_string()) {
                permissions.push_back(perm);
            }
        }
    }
    return permissions;
}

bool UserDAO::addUser(const UserModel& user) {
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    // 构建参数
    std::unordered_map<std::string, std::string> params;
    params["username"] = user.getUsername();
    params["password"] = user.getPassword();
    params["real_name"] = user.getRealName();
    params["role"] = user.getRole();
    params["is_active"] = user.getIsActive() ? "1" : "0";
    
    return dbManager->executeUpdate("INSERT INTO users (username, password, real_name, role, is_active) VALUES (?, ?, ?, ?, ?)", 
    {user.getUsername(), user.getPassword(), user.getRealName(), user.getRole(), user.getIsActive() ? "1" : "0"});
}

bool UserDAO::updateUser(const UserModel& user) {
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    std::unordered_map<std::string, std::string> params;
    params["id"] = std::to_string(user.getId());
    params["username"] = user.getUsername();
    params["password"] = user.getPassword();
    params["real_name"] = user.getRealName();
    params["role"] = user.getRole();
    params["is_active"] = user.getIsActive() ? "1" : "0";
    
    return dbManager->executeUpdate("UPDATE users SET username = ?, password = ?, real_name = ?, role = ?, is_active = ? WHERE id = ?", 
    {user.getUsername(), user.getPassword(), user.getRealName(), user.getRole(), user.getIsActive() ? "1" : "0", std::to_string(user.getId())});
}

bool UserDAO::deleteUser(int userId) {
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    std::unordered_map<std::string, std::string> params;
    params["id"] = std::to_string(userId);
    
    return dbManager->executeUpdate("DELETE FROM users WHERE id = ?", {std::to_string(userId)});
}

UserModel* UserDAO::getUserById(int userId) {
    DatabaseManager* dbManager = DatabaseManager::getInstance();
     // 调用executeQuery获取json结果
    json result = dbManager->executeQuery("SELECT * FROM users WHERE id = ?", {std::to_string(userId)});
    
    // 检查结果是否存在且包含数据
    if (result.contains("data") && result["data"].is_array() && !result["data"].empty()) {
        // 获取第一个用户记录
        json userData = result["data"][0];
        
        // 使用json数据创建UserModel对象
        return new UserModel(
            userData.value("id", 0),
            userData.value("username", ""),
            userData.value("password", ""),
            userData.value("real_name", ""),
            userData.value("role", ""),
            userData.value("is_active", "0") == "1"
        );
    }
    return nullptr;
}

UserModel* UserDAO::getUserByUsername(const std::string& username) {
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    // 使用位置占位符和vector<string>参数
    json result = dbManager->executeQuery("SELECT * FROM users WHERE username = ?", {username});
    
    // 检查结果是否存在且包含数据
    if (result.contains("data") && result["data"].is_array() && !result["data"].empty()) {
        // 获取第一个用户记录
        json userData = result["data"][0];
        
        // 使用json数据创建UserModel对象
        return new UserModel(
            userData.value("id", 0),
            userData.value("username", ""),
            userData.value("password", ""),
            userData.value("real_name", ""),
            userData.value("role", ""),
            userData.value("is_active", "0") == "1"
        );
    }
    return nullptr;
}

std::vector<UserModel> UserDAO::getAllUsers() {
    std::vector<UserModel> users;
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    
     // 使用json类型接收结果
    json result = dbManager->executeQuery("SELECT * FROM users", {});
    
    // 检查结果是否包含数据数组
    if (result.contains("data") && result["data"].is_array()) {
        // 遍历json数组中的每个用户记录
        for (const auto& userData : result["data"]) {
            // 为每个记录创建UserModel对象并添加到vector中
            users.push_back(UserModel(
                userData.value("id", 0),
                userData.value("username", ""),
                userData.value("password", ""),
                userData.value("real_name", ""),
                userData.value("role", ""),
                userData.value("is_active", "0") == "1"
            ));
        }
    }
    
    return users;
}