#include <dv-toolkit/io/reader.hpp>

int main() {
    namespace kit = dv::toolkit;

    // Initialize reader
    kit::io::MonoCameraReader reader("/path/to/aedat4");
    
    // Get offline MonoCameraData
    kit::MonoCameraData data = reader.loadData();
    
    // Check all basic types
    std::cout << data.events()   << std::endl;
    std::cout << data.frames()   << std::endl;
    std::cout << data.imus()     << std::endl;
    std::cout << data.triggers() << std::endl;

    // Get camera resolution
    // Can also use reader.getEventResolution()
    const auto resolution = reader.getResolution("events");
    if (resolution.has_value()) {
        std::cout << *resolution << std::endl;
    }

    return 0;
}