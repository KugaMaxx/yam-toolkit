#pragma once

#include "./base/event.hpp"
#include "./base/frame.hpp"
#include "./base/imu.hpp"
#include "./base/trigger.hpp"

#include <unordered_map>
#include <variant>

namespace dv::toolkit {

template<typename VariantType>
class UnifiedMap : public std::unordered_map<std::string, VariantType> {
public:
    VariantType &get(const std::string &name) {
        return this->at(name);
    }

    void set(const std::string &name, VariantType value) {
        (*this)[name] = value;
    }
};

using EVTS = EventStorage;
using FRME = FrameStorage;
using IMUS = IMUStorage;
using TRIG = TriggerStorage;

template<typename StandardCameraType>
class StandardCameraData : public UnifiedMap<std::variant<EVTS, FRME, IMUS, TRIG>> {
public:
    using UnifiedName = std::string;
    using UnifiedType = std::variant<EVTS, FRME, IMUS, TRIG>;

    void add(const StandardCameraType &other) {
        for (const auto &[key, value] : other) {
            this->add(key, value);
        }
    }

    void add(const std::string &name, const UnifiedType &store) {
        std::visit(
            [this, name](auto &&castedStore) {
                using Type = std::decay_t<decltype(castedStore)>;
                std::get<Type>((*this)[name]).add(castedStore);
            }, store);
    }

    [[nodiscard]] inline StandardCameraType sliceByNumber(const std::string &name, const size_t start) const {
        if (size(name) == 0 || start >= size(name)) {
            return StandardCameraType();
        }
        
        return sliceByNumber(name, start, size(name) - start);
    }

    [[nodiscard]] inline StandardCameraType sliceByNumber(const std::string &name, const size_t start, const size_t length) const {
        if (start + length > this->size(name)) {
            throw std::range_error("Slice exceeds Data range");
        }

        if (length == 0) {
            return StandardCameraType();
        }
        
        StandardCameraType slicedData;
        slicedData[name] = std::visit(
            [start, length](const auto &store) {
                return UnifiedType(store.slice(start, length));
            }, this->at(name));
        const auto timeWindow = slicedData.timeWindow(name);
        
        for (const auto &[key, value] : (*this)) {
            if (key == name) {
                continue;
            }
            slicedData[key] = std::visit(
                [timeWindow](const auto &store) {
                    return UnifiedType(store.sliceTime(
                        timeWindow.startTime,
                        timeWindow.endTime));
                }, value);
        }

        return slicedData;
    }

    [[nodiscard]] inline StandardCameraType sliceByTime(const std::string &name, const int64_t start) const {
        return sliceByTime(name, start, timeWindow(name).endTime + 1);
    }

    [[nodiscard]] inline StandardCameraType sliceByTime(const std::string &name, const int64_t start, const int64_t end) const {
        StandardCameraType slicedData;
        for (const auto &[key, value] : (*this)) {
            slicedData[key] = std::visit(
                [start, end](const auto &store) {
                    return UnifiedType(store.sliceTime(
                        start,
                        end));
                }, value);
        }

        return slicedData;
    }

	[[nodiscard]] inline size_t size(const std::string &name) const noexcept {
        return std::visit(
            [](const auto &store) {
                return store.size();
            }, this->at(name));
	}

	[[nodiscard]] inline dv::TimeWindow timeWindow(const std::string &name) const noexcept {
        return std::visit(
            [](const auto &store) {
                return store.timeWindow();
            }, this->at(name));
	}
};

class MonoCameraData : public StandardCameraData<MonoCameraData> {
public:
    MonoCameraData() {
        (*this)["events"]   = EventStorage();
        (*this)["frames"]   = FrameStorage();
        (*this)["imus"]     = IMUStorage();
        (*this)["triggers"] = TriggerStorage();
    };

    [[nodiscard]] EventStorage events() const {
        return std::get<EVTS>(this->at("events"));
    }

    [[nodiscard]] FrameStorage frames() const {
        return std::get<FRME>(this->at("frames"));
    }

    [[nodiscard]] IMUStorage imus() const {
        return std::get<IMUS>(this->at("imus"));
    }

    [[nodiscard]] TriggerStorage triggers() const {
        return std::get<TRIG>(this->at("triggers"));
    }
};

class CustomizedCameraData : public StandardCameraData<CustomizedCameraData> {
public:
    CustomizedCameraData() = default;
};

class StereoCameraData : public std::unordered_map<std::string, MonoCameraData> {
public:
    StereoCameraData() : std::unordered_map<std::string, MonoCameraData>() {
        (*this)["left"]  = MonoCameraData();
        (*this)["right"] = MonoCameraData();
    }
};

} // namespace dv::toolkit
