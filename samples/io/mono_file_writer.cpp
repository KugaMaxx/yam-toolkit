#include <dv-toolkit/core/core.hpp>
#include <dv-toolkit/io/reader.hpp>
#include <dv-toolkit/io/writer.hpp>
#include <dv-toolkit/simulation/generator.hpp>

int main() {
    namespace kit = dv::toolkit;

    // Enable literal time expression from the chrono library
    using namespace std::chrono_literals;
    
    // Initialize MonoCameraData
    kit::MonoCameraData data;

    // Initialize resolution
    const auto resolution = cv::Size(346, 260);

    // Create sample events
    data["events"] = kit::simulation::generateSampleEvents(resolution);

    // Initialize MonoCameraWriter
    kit::io::MonoCameraWriter writer("/path/to/file.aedat4", resolution);

    // Write MonoCameraData
    writer.writeData(data);

    return 0;
}