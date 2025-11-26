#include "studentService.h"
#include "studentDao.h"
#include "UserService.h"
#include <string>

StudentService* StudentService::instance = nullptr;

StudentService::StudentService() {
}

StudentService* StudentService::getInstance() {
    if (!instance) {
        instance = new StudentService();
    }
    return instance;
}

StudentService::~StudentService() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}

json StudentService::getStudentList(int page, int pageSize, int userId) {
    json response;
    
    // 权限检查
    UserService* userService = UserService::getInstance();
    if (!userService->checkPermission(userId, "view_student")) {
        response["success"] = false;
        response["message"] = "无权限查看学生信息";
        return response;
    }
    
    try {
        StudentDAO* studentDAO = StudentDAO::getInstance();
        std::vector<StudentModel> students = studentDAO->getStudentList(page, pageSize);
        int totalCount = studentDAO->getStudentCount();
        
        json studentsArray = json::array();
        for (const auto& student : students) {
            studentsArray.push_back(student.toJson());
        }
        
        response["success"] = true;
        response["students"] = studentsArray;
        response["total"] = totalCount;
        response["page"] = page;
        response["pageSize"] = pageSize;
    } catch (const std::exception& e) {
        response["success"] = false;
        response["message"] = "获取学生列表失败: " + std::string(e.what());
    }
    
    return response;
}

json StudentService::searchStudent(const std::string& keyword, int userId) {
    json response;
    
    // 权限检查
    UserService* userService = UserService::getInstance();
    if (!userService->checkPermission(userId, "view_student")) {
        response["success"] = false;
        response["message"] = "无权限搜索学生信息";
        return response;
    }
    
    try {
        StudentDAO* studentDAO = StudentDAO::getInstance();
        std::vector<StudentModel> students = studentDAO->searchStudent(keyword);
        
        json studentsArray = json::array();
        for (const auto& student : students) {
            studentsArray.push_back(student.toJson());
        }
        
        response["success"] = true;
        response["students"] = studentsArray;
    } catch (const std::exception& e) {
        response["success"] = false;
        response["message"] = "搜索学生失败: " + std::string(e.what());
    }
    
    return response;
}

json StudentService::getStudentDetail(int studentId, int userId) {
    json response;
    
    // 权限检查
    UserService* userService = UserService::getInstance();
    if (!userService->checkPermission(userId, "view_student")) {
        response["success"] = false;
        response["message"] = "无权限查看学生详情";
        return response;
    }
    
    try {
        StudentDAO* studentDAO = StudentDAO::getInstance();
        StudentModel* student = studentDAO->getStudentDetail(studentId);
        
        if (student) {
            response["success"] = true;
            response["student"] = student->toJson();
            delete student;
        } else {
            response["success"] = false;
            response["message"] = "学生不存在";
        }
    } catch (const std::exception& e) {
        response["success"] = false;
        response["message"] = "获取学生详情失败: " + std::string(e.what());
    }
    
    return response;
}

json StudentService::addStudent(const json& studentData, int userId) {
    json response;
    
    // 权限检查
    UserService* userService = UserService::getInstance();
    if (!userService->checkPermission(userId, "add_student")) {
        response["success"] = false;
        response["message"] = "无权限添加学生";
        return response;
    }
    
    try {
        // 参数校验
        if (!studentData.contains("studentId") || !studentData.contains("name")) {
            response["success"] = false;
            response["message"] = "缺少必要参数";
            return response;
        }
        
        // 构建学生模型
        StudentModel student(0, 
                           studentData["studentId"],
                           studentData["name"],
                           studentData.contains("gender") ? studentData["gender"] : "",
                           studentData.contains("birthday") ? studentData["birthday"] : "",
                           studentData.contains("phone") ? studentData["phone"] : "",
                           studentData.contains("email") ? studentData["email"] : "",
                           studentData.contains("department") ? studentData["department"] : "",
                           studentData.contains("major") ? studentData["major"] : "",
                           studentData.contains("className") ? studentData["className"] : "",
                           studentData.contains("enrollmentDate") ? studentData["enrollmentDate"] : "",
                           studentData.contains("status") ? studentData["status"] : "在读");
        
        StudentDAO* studentDAO = StudentDAO::getInstance();
        
        if (studentDAO->addStudent(student)) {
            response["success"] = true;
            response["message"] = "学生添加成功";
        } else {
            response["success"] = false;
            response["message"] = "学生添加失败";
        }
    } catch (const std::exception& e) {
        response["success"] = false;
        response["message"] = "添加学生失败: " + std::string(e.what());
    }
    
    return response;
}

json StudentService::updateStudent(const json& studentData, int userId) {
    json response;
    
    // 权限检查
    UserService* userService = UserService::getInstance();
    if (!userService->checkPermission(userId, "update_student")) {
        response["success"] = false;
        response["message"] = "无权限更新学生信息";
        return response;
    }
    
    try {
        if (!studentData.contains("id")) {
            response["success"] = false;
            response["message"] = "缺少学生ID";
            return response;
        }
        
        StudentModel student(studentData["id"],
                           studentData["studentId"],
                           studentData["name"],
                           studentData["gender"],
                           studentData["birthday"],
                           studentData["phone"],
                           studentData["email"],
                           studentData["department"],
                           studentData["major"],
                           studentData["className"],
                           studentData["enrollmentDate"],
                           studentData["status"]);
        
        StudentDAO* studentDAO = StudentDAO::getInstance();
        
        if (studentDAO->updateStudent(student)) {
            response["success"] = true;
            response["message"] = "学生信息更新成功";
        } else {
            response["success"] = false;
            response["message"] = "学生信息更新失败";
        }
    } catch (const std::exception& e) {
        response["success"] = false;
        response["message"] = "更新学生信息失败: " + std::string(e.what());
    }
    
    return response;
}

json StudentService::deleteStudent(int studentId, int userId) {
    json response;
    
    // 权限检查
    UserService* userService = UserService::getInstance();
    if (!userService->checkPermission(userId, "delete_student")) {
        response["success"] = false;
        response["message"] = "无权限删除学生";
        return response;
    }
    
    try {
        StudentDAO* studentDAO = StudentDAO::getInstance();
        
        // 检查学生是否存在
        StudentModel* student = studentDAO->getStudentDetail(studentId);
        if (!student) {
            response["success"] = false;
            response["message"] = "学生不存在";
            return response;
        }
        delete student;
        
        if (studentDAO->deleteStudent(studentId)) {
            response["success"] = true;
            response["message"] = "学生删除成功";
        } else {
            response["success"] = false;
            response["message"] = "学生删除失败";
        }
    } catch (const std::exception& e) {
        response["success"] = false;
        response["message"] = "删除学生失败: " + std::string(e.what());
    }
    
    return response;
}