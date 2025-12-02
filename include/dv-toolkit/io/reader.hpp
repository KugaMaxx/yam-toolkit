#include "../core/core.hpp"
#include <dv-processing/io/mono_camera_recording.hpp>


namespace fs = std::filesystem;
namespace kit = dv::toolkit;

namespace dv::toolkit::io {

class MonoCameraReader {
private:
    MonoCameraData _load_from_aedat4() {
        dv::toolkit::MonoCameraData data;

        // The isRunning() method of a dv::io::MonoCameraRecording returns false 
        // if any data-stream reaches End-of-file. Thus, data readout stops after 
        // any of the available data streams is complete.
        dv::io::MonoCameraRecording eventReader(mFilePath);
        while (eventReader.isRunning()) {
            if (eventReader.isEventStreamAvailable()) {
                if (const auto events = eventReader.getNextEventBatch(); events.has_value()) {
                    const auto packet = kit::EventPacket(events->toPacket().elements);
                    data.add("events", kit::EventStorage(packet));
                }
            }
        }

        dv::io::MonoCameraRecording frameReader(mFilePath);
        while (frameReader.isRunning()) {
            if (frameReader.isFrameStreamAvailable()) {
                if (const auto frame = frameReader.getNextFrame(); frame.has_value()) {
                    const auto packet = kit::FramePacket(std::vector<dv::Frame>{*frame});
                    data.add("frames", kit::FrameStorage(packet));
                }
            }
        }

        dv::io::MonoCameraRecording imuReader(mFilePath);
        while (imuReader.isRunning()) {
            if (imuReader.isImuStreamAvailable()) {
                if (const auto imus = imuReader.getNextImuBatch(); imus.has_value()) {
                    const auto packet = toolkit::IMUPacket(*imus);
                    data.add("imus", kit::IMUStorage(packet));
                }
            }
        }
        
        mEventResolution = eventReader.getEventResolution();
        mFrameResolution = frameReader.getFrameResolution();

        return data;
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
    std::optional<cv::Size> mEventResolution;
    std::optional<cv::Size> mFrameResolution;

public:
    MonoCameraReader(const fs::path &path) :
        mFilePath(path),
        mFileExtension(path.extension()),
        mEventResolution(std::nullopt),
        mFrameResolution(std::nullopt) {
    }

    MonoCameraData loadData() {
		switch (mSupportTable[mFileExtension.string()]) {
			case FileType::AEDAT4:
                return _load_from_aedat4();
                break;
			default:
				throw std::runtime_error("Unsupported file type");
		} 
    }

    [[nodiscard]] std::optional<cv::Size> getResolution(const std::string &name) const {
        if (name == "frame") {
            return mFrameResolution;
        }
        return mEventResolution;
	}

    [[nodiscard]] std::optional<cv::Size> getEventResolution() const {
        return mEventResolution;
	}

    [[nodiscard]] std::optional<cv::Size> getFrameResolution() const {
        return mFrameResolution;
	}
};

} // namespace dv::toolkit::io
