#include "src/date_expiration.h"
#include <iostream>

int main() {
    std::cout << "Testing Date Expiration System" << std::endl;
    std::cout << "=============================" << std::endl;
    
    std::cout << "Current date: " << GetCurrentDateString() << std::endl;
    std::cout << "Expiration date: " << GetExpirationDateString() << std::endl;
    
    bool expired = IsSoftwareExpired();
    std::cout << "Software expired: " << (expired ? "YES" : "NO") << std::endl;
    
    if (expired) {
        std::cout << "Force update screen should be shown!" << std::endl;
    } else {
        std::cout << "Software is still valid, normal flow should continue." << std::endl;
    }
    
    return 0;
}
