#include "application.h"
#include <iostream>
#include <stdexcept>

int main() {
    try {
        // Create and run the application
        Application app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
