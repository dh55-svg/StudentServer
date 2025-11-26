#pragma once
#include "UserModel.h"
#include <vector>
#include "json.hpp"

using json = nlohmann::json;

class UserService {
private:
    static UserService* instance;
    UserService();

public:
    static UserService* getInstance();
    ~UserService();

    // 用户认证服务
    json login(const std::string& username, const std::string& password);
    
    // 用户权限服务
    json getUserPermissions(int userId);
    
    // 用户管理服务
    json addUser(const json& userData);
    json updateUser(const json& userData);
    json deleteUser(int userId);
    json getUserById(int userId);
    json getAllUsers();
    
    // 权限检查
    bool checkPermission(int userId, const std::string& permission);
};