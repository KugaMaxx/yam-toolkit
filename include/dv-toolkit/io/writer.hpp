#include "../core/core.hpp"
#include <dv-processing/io/mono_camera_writer.hpp>


namespace fs = std::filesystem;
namespace kit = dv::toolkit;

namespace dv::toolkit::io {

using namespace std::chrono_literals;

class MonoCameraWriter {
private:
    void _write_to_aedat4(const MonoCameraData &data) {
        // dv::io::MonoCameraWriter::DAVISConfig config("DVXplorer_sample");
        dv::io::MonoCameraWriter::Config config = dv::io::MonoCameraWriter::DAVISConfig("test", mResolution);

        // Create the writer instance with the configuration structure
        dv::io::MonoCameraWriter writer(mFilePath, config);

        writer.writeEvents(data.events().toEventStore());
        
        for (const auto &frame : data.frames()) {
            writer.writeFrame(frame);
        }

        for (const auto &imu : data.imus()) {
            writer.writeImu(imu);
        }

        for (const auto &trigger : data.triggers()) {
            writer.writeTrigger(trigger);
        }
    }

    enum class FileType {
        AEDAT4,
        CSV,
    };

    std::unordered_map<std::string, FileType> mSupportTable = {
        {".aedat4", FileType::AEDAT4},
        {".csv",    FileType::CSV}
    };

    fs::path mFilePath;
    fs::path mFileExtension;
    cv::Size mResolution;

public:
    MonoCameraWriter(const fs::path &path, const cv::Size &resolution) :
        mFilePath(path),
        mFileExtension(path.extension()),
        mResolution(resolution) {
    }

    void writeData(const MonoCameraData &data) {
		switch (mSupportTable[mFileExtension.string()]) {
			case FileType::AEDAT4:
                _write_to_aedat4(data);
                break;
			default:
				throw std::runtime_error("Unsupported file type");
		}
    }
};

} // namespace dv::toolkit::io
