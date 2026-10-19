#ifndef USER_H
#define USER_H

#include <string>

class User {
public:
    // Age is an age-group code, not the user's exact age.
    User(int userId, char gender, int age, int occupation, std::string zipCode);
    static User fromDatLine(const std::string& line);
    std::string toDatLine() const;

    int getUserId() const { return userId_; }
    char getGender() const { return gender_; }
    int getAge() const { return age_; }
    int getOccupation() const { return occupation_; }
    const std::string& getZipCode() const { return zipCode_; }
    std::string getAgeDescription() const;
    std::string getOccupationDescription() const;

private:
    int userId_;
    char gender_;
    int age_;
    int occupation_;
    std::string zipCode_;
};

#endif
