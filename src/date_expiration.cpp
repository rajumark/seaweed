#include "date_expiration.h"
#include <ctime>
#include <string>
#include <sstream>
#include <iomanip>

// Expiration date constant in DD-MM-YYYY format
const std::string EXPIRATION_DATE = "16-05-2028";

bool IsSoftwareExpired() {
    // Get current date
    std::time_t now = std::time(nullptr);
    std::tm* current_tm = std::localtime(&now);
    
    // Parse expiration date
    std::stringstream ss(EXPIRATION_DATE);
    std::string day_str, month_str, year_str;
    
    std::getline(ss, day_str, '-');
    std::getline(ss, month_str, '-');
    std::getline(ss, year_str, '-');
    
    int exp_day = std::stoi(day_str);
    int exp_month = std::stoi(month_str);
    int exp_year = std::stoi(year_str);
    
    // Convert current date to comparable format
    int current_day = current_tm->tm_mday;
    int current_month = current_tm->tm_mon + 1; // tm_mon is 0-based
    int current_year = current_tm->tm_year + 1900; // tm_year is years since 1900
    
    // Compare dates
    if (current_year > exp_year) {
        return true; // Expired
    } else if (current_year == exp_year) {
        if (current_month > exp_month) {
            return true; // Expired
        } else if (current_month == exp_month) {
            if (current_day > exp_day) {
                return true; // Expired
            }
        }
    }
    
    return false; // Not expired
}

std::string GetCurrentDateString() {
    std::time_t now = std::time(nullptr);
    std::tm* current_tm = std::localtime(&now);
    
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << current_tm->tm_mday << "-"
       << std::setfill('0') << std::setw(2) << (current_tm->tm_mon + 1) << "-"
       << (current_tm->tm_year + 1900);
    
    return ss.str();
}

std::string GetExpirationDateString() {
    return EXPIRATION_DATE;
}
