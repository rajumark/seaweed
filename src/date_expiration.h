#pragma once
#include <string>

// Check if the software has expired based on current date vs expiration date
bool IsSoftwareExpired();

// Get current date as string in DD-MM-YYYY format
std::string GetCurrentDateString();

// Get expiration date as string in DD-MM-YYYY format
std::string GetExpirationDateString();
