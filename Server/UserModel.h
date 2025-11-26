#pragma once
#include <string>
#include "json.hpp"

using json = nlohmann::json;

class UserModel {
private:
    int id;
    std::string username;
    std::string password;
    std::string realName;
    std::string role;
    bool isActive;

public:
    UserModel();
    UserModel(int id, const std::string& username, const std::string& password, 
              const std::string& realName, const std::string& role, bool isActive);

    // Getters
    int getId() const { return id; }
    std::string getUsername() const { return username; }
    std::string getPassword() const { return password; }
    std::string getRealName() const { return realName; }
    std::string getRole() const { return role; }
    bool getIsActive() const { return isActive; }

    // Setters
    void setId(int id) { this->id = id; }
    void setUsername(const std::string& username) { this->username = username; }
    void setPassword(const std::string& password) { this->password = password; }
    void setRealName(const std::string& realName) { this->realName = realName; }
    void setRole(const std::string& role) { this->role = role; }
    void setIsActive(bool isActive) { this->isActive = isActive; }

    // JSON转换方法
    json toJson() const;
    static UserModel fromJson(const json& j);
};