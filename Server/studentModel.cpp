#include "studentModel.h"

StudentModel::StudentModel() : id(0) {
}

StudentModel::StudentModel(int id, const std::string& studentId, const std::string& name, 
                          const std::string& gender, const std::string& birthday, const std::string& phone,
                          const std::string& email, const std::string& department, const std::string& major,
                          const std::string& className, const std::string& enrollmentDate, const std::string& status)
    : id(id), studentId(studentId), name(name), gender(gender), birthday(birthday), phone(phone),
      email(email), department(department), major(major), className(className), enrollmentDate(enrollmentDate), status(status) {
}

json StudentModel::toJson() const {
    json j;
    j["id"] = id;
    j["studentId"] = studentId;
    j["name"] = name;
    j["gender"] = gender;
    j["birthday"] = birthday;
    j["phone"] = phone;
    j["email"] = email;
    j["department"] = department;
    j["major"] = major;
    j["className"] = className;
    j["enrollmentDate"] = enrollmentDate;
    j["status"] = status;
    return j;
}

StudentModel StudentModel::fromJson(const json& j) {
    StudentModel student;
    if (j.contains("id")) student.id = j["id"];
    if (j.contains("studentId")) student.studentId = j["studentId"];
    if (j.contains("name")) student.name = j["name"];
    if (j.contains("gender")) student.gender = j["gender"];
    if (j.contains("birthday")) student.birthday = j["birthday"];
    if (j.contains("phone")) student.phone = j["phone"];
    if (j.contains("email")) student.email = j["email"];
    if (j.contains("department")) student.department = j["department"];
    if (j.contains("major")) student.major = j["major"];
    if (j.contains("className")) student.className = j["className"];
    if (j.contains("enrollmentDate")) student.enrollmentDate = j["enrollmentDate"];
    if (j.contains("status")) student.status = j["status"];
    return student;
}