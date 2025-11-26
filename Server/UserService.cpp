#include "UserService.h"
#include "UserDao.h"
#include <string>

UserService* UserService::instance = nullptr;

UserService::UserService() {
}

UserService* UserService::getInstance() {
    if (!instance) {
        instance = new UserService();
    }
    return instance;
}

UserService::~UserService() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}

json UserService::login(const std::string& username, const std::string& password) {
    json response;
    
    // 参数校验
    if (username.empty() || password.empty()) {
        response["success"] = false;
        response["message"] = "用户名和密码不能为空";
        return response;
    }
    
    // 调用DAO层认证
    UserDAO* userDAO = UserDAO::getInstance();
    UserModel* user = userDAO->authenticateUser(username, password);
    
    if (user) {
        // 检查用户状态
        if (!user->getIsActive()) {
            response["success"] = false;
            response["message"] = "用户已被禁用";
            delete user;
            return response;
        }
        
        // 获取用户权限
        std::vector<std::string> permissions = userDAO->getUserPermissions(user->getId());
        
        // 构建响应
        response["success"] = true;
        response["message"] = "登录成功";
        response["user"] = user->toJson();
        response["permissions"] = permissions;
        
        delete user;
    } else {
        response["success"] = false;
        response["message"] = "用户名或密码错误";
    }
    
    return response;
}

json UserService::getUserPermissions(int userId) {
    json response;
    
    UserDAO* userDAO = UserDAO::getInstance();
    std::vector<std::string> permissions = userDAO->getUserPermissions(userId);
    
    response["success"] = true;
    response["permissions"] = permissions;
    
    return response;
}

json UserService::addUser(const json& userData) {
    json response;
    
    try {
        // 参数校验
        if (!userData.contains("username") || !userData.contains("password") || !userData.contains("realName")) {
            response["success"] = false;
            response["message"] = "缺少必要参数";
            return response;
        }
        
        // 构建用户模型
         UserModel user(0, 
                      userData.value("username", ""), 
                      userData.value("password", ""), 
                      userData.value("realName", ""), 
                      userData.value("role", "user"),
                      userData.value("isActive", true));
        
        UserDAO* userDAO = UserDAO::getInstance();
        
        // 检查用户名是否已存在
        if (userDAO->getUserByUsername(user.getUsername())) {
            response["success"] = false;
            response["message"] = "用户名已存在";
            return response;
        }
        
        // 添加用户
        if (userDAO->addUser(user)) {
            response["success"] = true;
            response["message"] = "用户添加成功";
        } else {
            response["success"] = false;
            response["message"] = "用户添加失败";
        }
    } catch (const std::exception& e) {
        response["success"] = false;
        response["message"] = "添加用户时发生错误: " + std::string(e.what());
    }
    
    return response;
}

json UserService::updateUser(const json& userData) {
    json response;
    
    try {
        if (!userData.contains("id")) {
            response["success"] = false;
            response["message"] = "缺少用户ID";
            return response;
        }
        
        UserModel user(userData["id"], 
                      userData["username"], 
                      userData["password"],
                      userData["realName"],
                      userData["role"],
                      userData["isActive"]);
        
        UserDAO* userDAO = UserDAO::getInstance();
        
        if (userDAO->updateUser(user)) {
            response["success"] = true;
            response["message"] = "用户更新成功";
        } else {
            response["success"] = false;
            response["message"] = "用户更新失败";
        }
    } catch (const std::exception& e) {
        response["success"] = false;
        response["message"] = "更新用户时发生错误: " + std::string(e.what());
    }
    
    return response;
}

json UserService::deleteUser(int userId) {
    json response;
    
    UserDAO* userDAO = UserDAO::getInstance();
    
    // 检查用户是否存在
    UserModel* user = userDAO->getUserById(userId);
    if (!user) {
        response["success"] = false;
        response["message"] = "用户不存在";
        return response;
    }
    delete user;
    
    if (userDAO->deleteUser(userId)) {
        response["success"] = true;
        response["message"] = "用户删除成功";
    } else {
        response["success"] = false;
        response["message"] = "用户删除失败";
    }
    
    return response;
}

json UserService::getUserById(int userId) {
    json response;
    
    UserDAO* userDAO = UserDAO::getInstance();
    UserModel* user = userDAO->getUserById(userId);
    
    if (user) {
        response["success"] = true;
        response["user"] = user->toJson();
        delete user;
    } else {
        response["success"] = false;
        response["message"] = "用户不存在";
    }
    
    return response;
}

json UserService::getAllUsers() {
    json response;
    
    UserDAO* userDAO = UserDAO::getInstance();
    std::vector<UserModel> users = userDAO->getAllUsers();
    
    json usersArray = json::array();
    for (const auto& user : users) {
        usersArray.push_back(user.toJson());
    }
    
    response["success"] = true;
    response["users"] = usersArray;
    
    return response;
}

bool UserService::checkPermission(int userId, const std::string& permission) {
    UserDAO* userDAO = UserDAO::getInstance();
    std::vector<std::string> permissions = userDAO->getUserPermissions(userId);
    
    // 检查是否有所需权限或是否为管理员
    return std::find(permissions.begin(), permissions.end(), permission) != permissions.end() ||
           std::find(permissions.begin(), permissions.end(), "admin") != permissions.end();
}