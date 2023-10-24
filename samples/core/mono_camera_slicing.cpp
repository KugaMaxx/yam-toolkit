#include <dv-toolkit/core/slicer.hpp>
#include <dv-toolkit/io/reader.hpp>

int main() {
    namespace kit = dv::toolkit;

    // Initialize reader
    kit::io::MonoCameraReader reader("/path/to/aedat4");
    
    // Get offline MonoCameraData
    kit::MonoCameraData data = reader.loadData();

    // Initialize slicer, it will have no jobs at this time
    kit::MonoCameraSlicer slicer;

    // Use this namespace to enable literal time expression from the chrono library
    using namespace std::chrono_literals;

    // Register this method to be called every 33 millisecond of events
    slicer.doEveryTimeInterval("events", 33ms, [](const kit::MonoCameraData &mono) {
        std::cout << mono.events() << std::endl;
    });

    // Register this method to be called every 2 elements of frames
    slicer.doEveryNumberOfElements("frames", 2, [](const kit::MonoCameraData &mono) {
        std::cout << mono.events() << std::endl;
    });

    // Now push the store into the slicer, the data contents within the store
    // can be arbitrary, the slicer implementation takes care of correct slicing
    // algorithm and calls the previously registered callbacks accordingly.
    slicer.accept(data);

    return 0;
}
