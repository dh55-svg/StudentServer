#pragma once
#include "studentModel.h"
#include <vector>
#include "json.hpp"

using json = nlohmann::json;

class StudentService {
private:
    static StudentService* instance;
    StudentService();

public:
    static StudentService* getInstance();
    ~StudentService();

    // 学生信息管理服务
    json getStudentList(int page, int pageSize, int userId);
    json searchStudent(const std::string& keyword, int userId);
    json getStudentDetail(int studentId, int userId);
    json addStudent(const json& studentData, int userId);
    json updateStudent(const json& studentData, int userId);
    json deleteStudent(int studentId, int userId);
};