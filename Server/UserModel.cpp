#include "UserModel.h"

UserModel::UserModel() : id(0), isActive(true) {
}

UserModel::UserModel(int id, const std::string& username, const std::string& password, 
                    const std::string& realName, const std::string& role, bool isActive)
    : id(id), username(username), password(password), realName(realName), role(role), isActive(isActive) {
}

json UserModel::toJson() const {
    json j;
    j["id"] = id;
    j["username"] = username;
    j["realName"] = realName;
    j["role"] = role;
    j["isActive"] = isActive;
    // 不包含密码
    return j;
}

UserModel UserModel::fromJson(const json& j) {
    UserModel user;
    if (j.contains("id")) user.id = j["id"];
    if (j.contains("username")) user.username = j["username"];
    if (j.contains("password")) user.password = j["password"];
    if (j.contains("realName")) user.realName = j["realName"];
    if (j.contains("role")) user.role = j["role"];
    if (j.contains("isActive")) user.isActive = j["isActive"];
    return user;
}