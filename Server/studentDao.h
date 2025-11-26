#pragma once
#include "studentModel.h"
#include <vector>
#include "json.hpp"

using json = nlohmann::json;

class StudentDAO {
private:
    static StudentDAO* instance;
    StudentDAO();

public:
    static StudentDAO* getInstance();
    ~StudentDAO();

    // 学生信息管理
    std::vector<StudentModel> getStudentList(int page, int pageSize);
    std::vector<StudentModel> searchStudent(const std::string& keyword);
    StudentModel* getStudentDetail(int studentId);
    bool addStudent(const StudentModel& student);
    bool updateStudent(const StudentModel& student);
    bool deleteStudent(int studentId);
    int getStudentCount();
};