#pragma once
#include "UserModel.h"
#include <vector>

class UserDAO {
private:
    // 单例模式
    static UserDAO* instance;
    UserDAO();

public:
    static UserDAO* getInstance();
    ~UserDAO();

    // 用户认证
    UserModel* authenticateUser(const std::string& username, const std::string& password);
    
    // 获取用户权限
    std::vector<std::string> getUserPermissions(int userId);
    
    // 用户管理
    bool addUser(const UserModel& user);
    bool updateUser(const UserModel& user);
    bool deleteUser(int userId);
    UserModel* getUserById(int userId);
    UserModel* getUserByUsername(const std::string& username);
    std::vector<UserModel> getAllUsers();
};