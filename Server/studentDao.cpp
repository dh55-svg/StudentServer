#include "studentDao.h"
#include "DatabaseManager.h"
#include <mysql/mysql.h>

StudentDAO* StudentDAO::instance = nullptr;

StudentDAO::StudentDAO() {
}

StudentDAO* StudentDAO::getInstance() {
    if (!instance) {
        instance = new StudentDAO();
    }
    return instance;
}

StudentDAO::~StudentDAO() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}

std::vector<StudentModel> StudentDAO::getStudentList(int page, int pageSize) {
    std::vector<StudentModel> students;
    DatabaseManager * dbManager = DatabaseManager::getInstance();
    
    std::unordered_map<std::string, std::string> params;
    params["offset"] = std::to_string((page - 1) * pageSize);
    params["limit"] = std::to_string(pageSize);
    
    json result = dbManager->executeQuery(
        "SELECT * FROM students LIMIT ?, ?", 
        params
    );
    
    // 检查result是否包含data字段并且是数组
    if (result.contains("data") && result["data"].is_array()) {
        for (const auto& item : result["data"]) {
            students.push_back(StudentModel(
                item.value("id", 0),
                item.value("student_id", ""),
                item.value("name", ""),
                item.value("gender", ""),
                item.value("birthday", ""),
                item.value("phone", ""),
                item.value("email", ""),
                item.value("department", ""),
                item.value("major", ""),
                item.value("class_name", ""),
                item.value("enrollment_date", ""),
                item.value("status", "")
            ));
        }
    }
    return students;
}

std::vector<StudentModel> StudentDAO::searchStudent(const std::string& keyword) {
    std::vector<StudentModel> students;
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    
    std::unordered_map<std::string, std::string> params;
    params["keyword"] = "%" + keyword + "%";
    
    json result = dbManager->executeQuery(
        "SELECT * FROM students WHERE student_id LIKE ? OR name LIKE ? OR department LIKE ? OR major LIKE ?", 
        params
    );
    
    // 检查result是否包含data字段并且是数组
    if (result.contains("data") && result["data"].is_array()) {
        for (const auto& item : result["data"]) {
            students.push_back(StudentModel(
                item.value("id", 0),
                item.value("student_id", ""),
                item.value("name", ""),
                item.value("gender", ""),
                item.value("birthday", ""),
                item.value("phone", ""),
                item.value("email", ""),
                item.value("department", ""),
                item.value("major", ""),
                item.value("class_name", ""),
                item.value("enrollment_date", ""),
                item.value("status", "")
            ));
        }
    }
    return students;
}

StudentModel* StudentDAO::getStudentDetail(int studentId) {
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    std::unordered_map<std::string, std::string> params;
    params["id"] = std::to_string(studentId);
    
    json result = dbManager->executeQuery("SELECT * FROM students WHERE id = ?", params);
    
    // 检查result是否包含data字段、是否是数组且不为空
    if (result.contains("data") && result["data"].is_array() && !result["data"].empty()) {
        const auto& item = result["data"][0];
        return new StudentModel(
            item.value("id", 0),
            item.value("student_id", ""),
            item.value("name", ""),
            item.value("gender", ""),
            item.value("birthday", ""),
            item.value("phone", ""),
            item.value("email", ""),
            item.value("department", ""),
            item.value("major", ""),
            item.value("class_name", ""),
            item.value("enrollment_date", ""),
            item.value("status", "")
        );
    }
    return nullptr;
}

bool StudentDAO::addStudent(const StudentModel& student) {
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    std::unordered_map<std::string, std::string> params;
    params["student_id"] = student.getStudentId();
    params["name"] = student.getName();
    params["gender"] = student.getGender();
    params["birthday"] = student.getBirthday();
    params["phone"] = student.getPhone();
    params["email"] = student.getEmail();
    params["department"] = student.getDepartment();
    params["major"] = student.getMajor();
    params["class_name"] = student.getClassName();
    params["enrollment_date"] = student.getEnrollmentDate();
    params["status"] = student.getStatus();
    
    return dbManager->executeUpdate(
        "INSERT INTO students (student_id, name, gender, birthday, phone, email, department, major, class_name, enrollment_date, status) "
        "VALUES (:student_id, :name, :gender, :birthday, :phone, :email, :department, :major, :class_name, :enrollment_date, :status)",
        params
    );
}

bool StudentDAO::updateStudent(const StudentModel& student) {
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    std::unordered_map<std::string, std::string> params;
    params["id"] = std::to_string(student.getId());
    params["student_id"] = student.getStudentId();
    params["name"] = student.getName();
    params["gender"] = student.getGender();
    params["birthday"] = student.getBirthday();
    params["phone"] = student.getPhone();
    params["email"] = student.getEmail();
    params["department"] = student.getDepartment();
    params["major"] = student.getMajor();
    params["class_name"] = student.getClassName();
    params["enrollment_date"] = student.getEnrollmentDate();
    params["status"] = student.getStatus();
    
    return dbManager->executeUpdate(
        "UPDATE students SET student_id = :student_id, name = :name, gender = :gender, birthday = :birthday, "
        "phone = :phone, email = :email, department = :department, major = :major, class_name = :class_name, "
        "enrollment_date = :enrollment_date, status = :status WHERE id = :id",
        params
    );
}

bool StudentDAO::deleteStudent(int studentId) {
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    std::unordered_map<std::string, std::string> params;
    params["id"] = std::to_string(studentId);
    
    return dbManager->executeUpdate("DELETE FROM students WHERE id = :id", params);
}

int StudentDAO::getStudentCount() {
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    json result = dbManager->executeQuery("SELECT COUNT(*) as count FROM students", {});
    
    // 检查result是否包含data字段、是否是数组且不为空
    if (result.contains("data") && result["data"].is_array() && !result["data"].empty()) {
        const auto& item = result["data"][0];
        // 使用value方法安全地获取count值，并提供默认值0
        return item.value("count", 0);
    }
    return 0;
}