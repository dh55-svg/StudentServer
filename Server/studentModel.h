#pragma once
#include <string>
#include "json.hpp"

using json = nlohmann::json;

class StudentModel {
private:
    int id;
    std::string studentId;
    std::string name;
    std::string gender;
    std::string birthday;
    std::string phone;
    std::string email;
    std::string department;
    std::string major;
    std::string className;
    std::string enrollmentDate;
    std::string status;

public:
    StudentModel();
    StudentModel(int id, const std::string& studentId, const std::string& name, 
                const std::string& gender, const std::string& birthday, const std::string& phone,
                const std::string& email, const std::string& department, const std::string& major,
                const std::string& className, const std::string& enrollmentDate, const std::string& status);

    // Getters
    int getId() const { return id; }
    std::string getStudentId() const { return studentId; }
    std::string getName() const { return name; }
    std::string getGender() const { return gender; }
    std::string getBirthday() const { return birthday; }
    std::string getPhone() const { return phone; }
    std::string getEmail() const { return email; }
    std::string getDepartment() const { return department; }
    std::string getMajor() const { return major; }
    std::string getClassName() const { return className; }
    std::string getEnrollmentDate() const { return enrollmentDate; }
    std::string getStatus() const { return status; }

    // Setters
    void setId(int id) { this->id = id; }
    void setStudentId(const std::string& studentId) { this->studentId = studentId; }
    void setName(const std::string& name) { this->name = name; }
    void setGender(const std::string& gender) { this->gender = gender; }
    void setBirthday(const std::string& birthday) { this->birthday = birthday; }
    void setPhone(const std::string& phone) { this->phone = phone; }
    void setEmail(const std::string& email) { this->email = email; }
    void setDepartment(const std::string& department) { this->department = department; }
    void setMajor(const std::string& major) { this->major = major; }
    void setClassName(const std::string& className) { this->className = className; }
    void setEnrollmentDate(const std::string& enrollmentDate) { this->enrollmentDate = enrollmentDate; }
    void setStatus(const std::string& status) { this->status = status; }

    // JSON转换方法
    json toJson() const;
    static StudentModel fromJson(const json& j);
};