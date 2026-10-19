#include "User.h"
#include "DataRecord.h"
#include <utility>

User::User(int userId, char gender, int age, int occupation, std::string zipCode)
    : userId_(userId), gender_(gender), age_(age), occupation_(occupation),
      zipCode_(std::move(zipCode))
{
    if (userId < 1 || userId > 6040) throw std::invalid_argument("User ID must be 1–6040");
    if (gender != 'M' && gender != 'F') throw std::invalid_argument("Gender must be M or F");
    if (age != 1 && age != 18 && age != 25 && age != 35 && age != 45 && age != 50 && age != 56)
        throw std::invalid_argument("Invalid age-group code");
    if (occupation < 0 || occupation > 20) throw std::invalid_argument("Occupation must be 0–20");
    if (zipCode_.empty() || zipCode_.find_first_of(":\r\n") != std::string::npos)
        throw std::invalid_argument("Invalid ZIP code");
}

User User::fromDatLine(const std::string& line)
{
    const auto f = data_record::fields(line, 5);
    if (f[1].size() != 1) throw std::invalid_argument("Gender must be a single character");
    return User(data_record::number<int>(f[0]), f[1][0], data_record::number<int>(f[2]),
                data_record::number<int>(f[3]), f[4]);
}

std::string User::toDatLine() const
{
    return std::to_string(userId_) + "::" + gender_ + "::" + std::to_string(age_) + "::" +
           std::to_string(occupation_) + "::" + zipCode_;
}

std::string User::getAgeDescription() const
{
    switch (age_) {
        case 1: return "Under 18";
        case 18: return "18-24";
        case 25: return "25-34";
        case 35: return "35-44";
        case 45: return "45-49";
        case 50: return "50-55";
        default: return "56+";
    }
}

std::string User::getOccupationDescription() const
{
    static const char* descriptions[] = {
        "other or not specified", "academic/educator", "artist", "clerical/admin",
        "college/grad student", "customer service", "doctor/health care",
        "executive/managerial", "farmer", "homemaker", "K-12 student", "lawyer",
        "programmer", "retired", "sales/marketing", "scientist", "self-employed",
        "technician/engineer", "tradesman/craftsman", "unemployed", "writer"
    };
    return descriptions[occupation_];
}
