#include <utility>

#include <pybind11/chrono.h>
#include <pybind11/complex.h>
#include <pybind11/eigen.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include <dv-toolkit/toolkit.hpp>
#include <dv-processing/processing.hpp>
#include "../external/pybind11_opencv_numpy/ndarray_converter.h"

namespace py  = pybind11;
namespace kit = dv::toolkit;

namespace pybind11::detail {

template<>
struct type_caster<cv::Size> {
	PYBIND11_TYPE_CASTER(cv::Size, _("tuple_xy"));

	bool load(handle obj, bool) {
		if (!py::isinstance<py::tuple>(obj)) {
			std::logic_error("Size(width,height) should be a tuple!");
			return false;
		}

		auto pt = reinterpret_borrow<py::tuple>(obj);
		if (pt.size() != 2) {
			std::logic_error("Size(width,height) tuple should be size of 2");
			return false;
		}

		value = cv::Size(pt[0].cast<int>(), pt[1].cast<int>());
		return true;
	}

	static handle cast(const cv::Size &resolution, return_value_policy, handle) {
		return py::make_tuple(resolution.width, resolution.height).release();
	}
};

} // namespace pybind11::detail

PYBIND11_MODULE(_lib_toolkit, m) {
	using pybind11::operator""_a;

	NDArrayConverter::init_numpy();

	py::class_<std::filesystem::path>(m, "Path")
		.def(py::init<std::string>(), "path"_a);
	py::implicitly_convertible<std::string, std::filesystem::path>();

	py::class_<dv::TimeWindow>(m, "TimeWindow")
        .def(py::init<int64_t, dv::Duration>())
        .def(py::init<int64_t, int64_t>())
        .def_readwrite("startTime", &dv::TimeWindow::startTime)
        .def_readwrite("endTime", &dv::TimeWindow::endTime)
        .def("duration", &dv::TimeWindow::duration);

	py::class_<kit::EventPacket>(m, "EventPacket")
		.def(py::init<>())
		.def(py::init<const dv::cvector<dv::Event> &>(), "events"_a)
		.def_readwrite("elements", &kit::EventPacket::elements)
		.def("__array__",
			 [](const kit::EventPacket &self) {
				py::list names;
				names.append("timestamp");
				names.append("x");
				names.append("y");
				names.append("polarity");
				py::list formats;
				formats.append("<i8");
				formats.append("<i2");
				formats.append("<i2");
				formats.append("<i1");
				py::list offsets;
				offsets.append(0);
				offsets.append(8);
				offsets.append(10);
				offsets.append(12);
				return py::array(
					py::dtype(names, formats, offsets, sizeof(dv::Event)), self.elements.size(), self.elements.data());
			 })
		.def("__repr__",
			[](const kit::EventPacket &self) {
				std::stringstream ss;
				ss << self;
				return ss.str();
			})
		.def_static("GetFullyQualifiedName", &kit::EventPacket::GetFullyQualifiedName);
	
	py::class_<kit::FramePacket>(m, "FramePacket")
		.def(py::init<>())
		.def(py::init<const dv::cvector<dv::Frame> &>(), "frames"_a)
		.def_readwrite("elements", &kit::FramePacket::elements)
		.def("__repr__",
			[](const kit::FramePacket &self) {
				std::stringstream ss;
				ss << self;
				return ss.str();
			})
		.def_static("GetFullyQualifiedName", &kit::FramePacket::GetFullyQualifiedName);

	py::class_<kit::IMUPacket>(m, "IMUPacket")
		.def(py::init<>())
		.def(py::init<const dv::cvector<dv::IMU> &>(), "imus"_a)
		.def_readwrite("elements", &kit::IMUPacket::elements)
		.def("__repr__",
			[](const kit::IMUPacket &self) {
				std::stringstream ss;
				ss << self;
				return ss.str();
			})
		.def_static("GetFullyQualifiedName", &kit::IMUPacket::GetFullyQualifiedName);

	py::class_<kit::TriggerPacket>(m, "TriggerPacket")
		.def(py::init<>())
		.def(py::init<const dv::cvector<dv::Trigger> &>(), "triggers"_a)
		.def_readwrite("elements", &kit::TriggerPacket::elements)
		.def("__repr__",
			[](const kit::TriggerPacket &self) {
				std::stringstream ss;
				ss << self;
				return ss.str();
		})
		.def_static("GetFullyQualifiedName", &kit::TriggerPacket::GetFullyQualifiedName);

    py::class_<kit::EventStorage>(m, "EventStorage")
        .def(py::init<>())
        .def(py::init<kit::EventPacket&>())
		.def("numpy", 
            [](const kit::EventStorage &self) {
                // Convert into a continuous memory buffer
                auto *events = new kit::EventPacket(self.toPacket());
                // Deallocator
                py::capsule deallocate(events, [](void *ptr) {
                    delete reinterpret_cast<dv::EventPacket *>(ptr);
                });
                // Register field
                py::list names;
                names.append("timestamp");
                names.append("x");
                names.append("y");
                names.append("polarity");
                py::list formats;
                formats.append("<i8");
                formats.append("<i2");
                formats.append("<i2");
                formats.append("<i1");
                py::list offsets;
                offsets.append(0);
                offsets.append(8);
                offsets.append(10);
                offsets.append(12);
                return py::array(py::dtype(names, formats, offsets, sizeof(dv::Event)), events->elements.size(), events->elements.data(), deallocate);
            })
        .def("__repr__",
            [](const kit::EventStorage &self) {
                std::stringstream ss;
                ss << self;
                return ss.str();
            })
		.def("__len__", &kit::EventStorage::size)
		.def("__getitem__", &kit::EventStorage::at)
        .def("__iter__",
			[](const kit::EventStorage &self) {
				return py::make_iterator(self.begin(), self.end());
			}, py::keep_alive<0, 1>())
        .def("add",
			[](kit::EventStorage &self, const kit::EventStorage &other) {
				return self.add(other);
			}, "other"_a)
		.def("slice",
			[](const kit::EventStorage &self, const size_t start) {
				return self.slice(start);
			}, "start"_a)		
		.def("slice",
			[](const kit::EventStorage &self, const size_t start, const size_t length) {
				return self.slice(start, length);
			}, "start"_a, "length"_a)
		.def("sliceTime",
			[](const kit::EventStorage &self, const int64_t startTime) {
				return self.sliceTime(startTime);
			}, "startTime"_a)		
		.def("sliceTime",
			[](const kit::EventStorage &self, const int64_t startTime, const int64_t endTime) {
				return self.sliceTime(startTime, endTime);
			}, "startTime"_a, "endTime"_a)
		.def("downSample", &kit::EventStorage::downSample)
        .def("copy", &kit::EventStorage::copy)
		.def("size", &kit::EventStorage::size)
		.def("getLowestTime", &kit::EventStorage::getLowestTime)
		.def("getHighestTime", &kit::EventStorage::getHighestTime)
		.def("at", &kit::EventStorage::at, "index"_a)
		.def("isEmpty", &kit::EventStorage::isEmpty)
		.def("erase", &kit::EventStorage::erase, "start"_a, "length"_a)
		.def("eraseTime", &kit::EventStorage::eraseTime, "startTime"_a, "endTime"_a)
		.def("front", &kit::EventStorage::front)
		.def("back", &kit::EventStorage::back)
		.def("retainDuration", &kit::EventStorage::retainDuration, "duration"_a)
		.def("duration", &kit::EventStorage::duration)
		.def("timeWindow", &kit::EventStorage::timeWindow)
		.def("rate", &kit::EventStorage::rate)
        .def("push_back",
			[](kit::EventStorage &self, const int64_t timestamp, const int16_t x, const int16_t y, const bool polarity) {
				return self.emplace_back(timestamp, x, y, polarity);
			}, "timestamp"_a, "x"_a, "y"_a, "polarity"_a)
		.def("emplace_back",
			[](kit::EventStorage &self, const int64_t timestamp, const int16_t x, const int16_t y, const bool polarity) {
				return self.emplace_back(timestamp, x, y, polarity);
			}, "timestamp"_a, "x"_a, "y"_a, "polarity"_a)
		.def("timestamps", &kit::EventStorage::timestamps)
		.def("xs", &kit::EventStorage::xs)
		.def("ys", &kit::EventStorage::ys)
		.def("coordinates", &kit::EventStorage::coordinates)
		.def("polarities", &kit::EventStorage::polarities)
		.def("toEventStore", &kit::EventStorage::toEventStore);

    py::class_<kit::FrameStorage>(m, "FrameStorage")
        .def(py::init<>())
        .def(py::init<kit::FramePacket&>())
        .def("__repr__",
            [](const kit::FrameStorage &self) {
                std::stringstream ss;
                ss << self;
                return ss.str();
            })
		.def("__len__", &kit::FrameStorage::size)
		.def("__getitem__", &kit::FrameStorage::at)
        .def("__iter__",
			[](const kit::FrameStorage &self) {
				return py::make_iterator(self.begin(), self.end());
			}, py::keep_alive<0, 1>())
        .def("add",
			[](kit::FrameStorage &self, const kit::FrameStorage &other) {
				return self.add(other);
			}, "other"_a)
		.def("slice",
			[](const kit::FrameStorage &self, const size_t start) {
				return self.slice(start);
			}, "start"_a)		
		.def("slice",
			[](const kit::FrameStorage &self, const size_t start, const size_t length) {
				return self.slice(start, length);
			}, "start"_a, "length"_a)
		.def("sliceTime",
			[](const kit::FrameStorage &self, const int64_t startTime) {
				return self.sliceTime(startTime);
			}, "startTime"_a)		
		.def("sliceTime",
			[](const kit::FrameStorage &self, const int64_t startTime, const int64_t endTime) {
				return self.sliceTime(startTime, endTime);
			}, "startTime"_a, "endTime"_a)
        .def("copy", &kit::FrameStorage::copy)
		.def("size", &kit::FrameStorage::size)
		.def("getLowestTime", &kit::FrameStorage::getLowestTime)
		.def("getHighestTime", &kit::FrameStorage::getHighestTime)
		.def("at", &kit::FrameStorage::at, "index"_a)
		.def("isEmpty", &kit::FrameStorage::isEmpty)
		.def("erase", &kit::FrameStorage::erase, "start"_a, "length"_a)
		.def("eraseTime", &kit::FrameStorage::eraseTime, "startTime"_a, "endTime"_a)
		.def("front", &kit::FrameStorage::front)
		.def("back", &kit::FrameStorage::back)
		.def("retainDuration", &kit::FrameStorage::retainDuration, "duration"_a)
		.def("duration", &kit::FrameStorage::duration)
		.def("timeWindow", &kit::FrameStorage::timeWindow)
		.def("rate", &kit::FrameStorage::rate)
        .def("push_back",
			[](kit::FrameStorage &self, const int64_t timestamp, const cv::Mat &image) {
				return self.emplace_back(timestamp, image);
			}, "timestamp"_a, "image"_a)
		.def("emplace_back",
			[](kit::FrameStorage &self, const int64_t timestamp, const cv::Mat &image) {
				return self.emplace_back(timestamp, image);
			}, "timestamp"_a, "image"_a);

    py::class_<kit::IMUStorage>(m, "IMUStorage")
        .def(py::init<>())
        .def(py::init<kit::IMUPacket&>())
        .def("__repr__",
            [](const kit::IMUStorage &self) {
                std::stringstream ss;
                ss << self;
                return ss.str();
            })
		.def("__len__", &kit::IMUStorage::size)
		.def("__getitem__", &kit::IMUStorage::at)
        .def("__iter__",
			[](const kit::IMUStorage &self) {
				return py::make_iterator(self.begin(), self.end());
			}, py::keep_alive<0, 1>())
        .def("add",
			[](kit::IMUStorage &self, const kit::IMUStorage &other) {
				return self.add(other);
			}, "other"_a)
		.def("slice",
			[](const kit::IMUStorage &self, const size_t start) {
				return self.slice(start);
			}, "start"_a)		
		.def("slice",
			[](const kit::IMUStorage &self, const size_t start, const size_t length) {
				return self.slice(start, length);
			}, "start"_a, "length"_a)
		.def("sliceTime",
			[](const kit::IMUStorage &self, const int64_t startTime) {
				return self.sliceTime(startTime);
			}, "startTime"_a)		
		.def("sliceTime",
			[](const kit::IMUStorage &self, const int64_t startTime, const int64_t endTime) {
				return self.sliceTime(startTime, endTime);
			}, "startTime"_a, "endTime"_a)
        .def("copy", &kit::IMUStorage::copy)
		.def("size", &kit::IMUStorage::size)
		.def("getLowestTime", &kit::IMUStorage::getLowestTime)
		.def("getHighestTime", &kit::IMUStorage::getHighestTime)
		.def("at", &kit::IMUStorage::at, "index"_a)
		.def("isEmpty", &kit::IMUStorage::isEmpty)
		.def("erase", &kit::IMUStorage::erase, "start"_a, "length"_a)
		.def("eraseTime", &kit::IMUStorage::eraseTime, "startTime"_a, "endTime"_a)
		.def("front", &kit::IMUStorage::front)
		.def("back", &kit::IMUStorage::back)
		.def("retainDuration", &kit::IMUStorage::retainDuration, "duration"_a)
		.def("duration", &kit::IMUStorage::duration)
		.def("timeWindow", &kit::IMUStorage::timeWindow)
		.def("rate", &kit::IMUStorage::rate);

    py::class_<kit::TriggerStorage>(m, "TriggerStorage")
        .def(py::init<>())
        .def(py::init<kit::TriggerPacket&>())
        .def("__repr__",
            [](const kit::TriggerStorage &self) {
                std::stringstream ss;
                ss << self;
                return ss.str();
            })
		.def("__len__", &kit::TriggerStorage::size)
		.def("__getitem__", &kit::TriggerStorage::at)
        .def("__iter__",
			[](const kit::TriggerStorage &self) {
				return py::make_iterator(self.begin(), self.end());
			}, py::keep_alive<0, 1>())
        .def("add",
			[](kit::TriggerStorage &self, const kit::TriggerStorage &other) {
				return self.add(other);
			}, "other"_a)
		.def("slice",
			[](const kit::TriggerStorage &self, const size_t start) {
				return self.slice(start);
			}, "start"_a)		
		.def("slice",
			[](const kit::TriggerStorage &self, const size_t start, const size_t length) {
				return self.slice(start, length);
			}, "start"_a, "length"_a)
		.def("sliceTime",
			[](const kit::TriggerStorage &self, const int64_t startTime) {
				return self.sliceTime(startTime);
			}, "startTime"_a)		
		.def("sliceTime",
			[](const kit::TriggerStorage &self, const int64_t startTime, const int64_t endTime) {
				return self.sliceTime(startTime, endTime);
			}, "startTime"_a, "endTime"_a)
        .def("copy", &kit::TriggerStorage::copy)
		.def("size", &kit::TriggerStorage::size)
		.def("getLowestTime", &kit::TriggerStorage::getLowestTime)
		.def("getHighestTime", &kit::TriggerStorage::getHighestTime)
		.def("at", &kit::TriggerStorage::at, "index"_a)
		.def("isEmpty", &kit::TriggerStorage::isEmpty)
		.def("erase", &kit::TriggerStorage::erase, "start"_a, "length"_a)
		.def("eraseTime", &kit::TriggerStorage::eraseTime, "startTime"_a, "endTime"_a)
		.def("front", &kit::TriggerStorage::front)
		.def("back", &kit::TriggerStorage::back)
		.def("retainDuration", &kit::TriggerStorage::retainDuration, "duration"_a)
		.def("duration", &kit::TriggerStorage::duration)
		.def("timeWindow", &kit::TriggerStorage::timeWindow)
		.def("rate", &kit::TriggerStorage::rate);

	py::class_<kit::MonoCameraData>(m, "MonoCameraData")
		.def(py::init<>())
		.def("__getitem__", &kit::MonoCameraData::get, py::return_value_policy::reference)
		.def("__setitem__", &kit::MonoCameraData::set)
		.def("get", &kit::MonoCameraData::get, py::return_value_policy::reference)
		.def("set", &kit::MonoCameraData::set)
		.def("add",
			 [](kit::MonoCameraData &self, const kit::MonoCameraData &other) {
				return self.add(other);
			 }, "other"_a)
		.def("add",
			 [](kit::MonoCameraData &self, const std::string &name, const kit::MonoCameraData::UnifiedType &store) {
				return self.add(name, store);
			 }, "name"_a, "store"_a)
		.def("sliceByNumber",
			 [](const kit::MonoCameraData &self, const std::string &name, const size_t start) {
				return self.sliceByNumber(name, start);
			 }, "name"_a, "start"_a)
		.def("sliceByNumber",
			 [](const kit::MonoCameraData &self, const std::string &name, const size_t start, const size_t length) {
				return self.sliceByNumber(name, start, length);
			 }, "name"_a, "start"_a, "length"_a)
		.def("sliceByNumber",
			 [](const kit::MonoCameraData &self, const std::string &name, const size_t start) {
				return self.sliceByNumber(name, start);
			 }, "name"_a, "start"_a)
		.def("sliceByNumber",
			 [](const kit::MonoCameraData &self, const std::string &name, const size_t start, const size_t length) {
				return self.sliceByNumber(name, start, length);
			 }, "name"_a, "start"_a, "length"_a)
		.def("sliceByTime",
			 [](const kit::MonoCameraData &self, const std::string &name, const int64_t start) {
				return self.sliceByTime(name, start);
			 }, "name"_a, "start"_a)
		.def("sliceByTime",
			 [](const kit::MonoCameraData &self, const std::string &name, const int64_t start, const int64_t end) {
				return self.sliceByTime(name, start, end);
			 }, "name"_a, "start"_a, "end"_a)
		.def("size", &kit::MonoCameraData::size)
		.def("timeWindow", &kit::MonoCameraData::timeWindow)
		.def("events", &kit::MonoCameraData::events)
		.def("frames", &kit::MonoCameraData::frames)
		.def("imus", &kit::MonoCameraData::imus)
		.def("triggers", &kit::MonoCameraData::triggers);

	py::class_<kit::CustomizedCameraData>(m, "CustomizedCameraData")
		.def(py::init<>())
		.def("__getitem__", &kit::CustomizedCameraData::get, py::return_value_policy::reference)
		.def("__setitem__", &kit::CustomizedCameraData::set)
		.def("get", &kit::CustomizedCameraData::get, py::return_value_policy::reference)
		.def("set", &kit::CustomizedCameraData::set)
		.def("add",
			 [](kit::CustomizedCameraData &self, const kit::CustomizedCameraData &other) {
				return self.add(other);
			 }, "other"_a)
		.def("add",
			 [](kit::CustomizedCameraData &self, const std::string &name, const kit::CustomizedCameraData::UnifiedType &store) {
				return self.add(name, store);
			 }, "name"_a, "store"_a)
		.def("sliceByNumber",
			 [](const kit::CustomizedCameraData &self, const std::string &name, const size_t start) {
				return self.sliceByNumber(name, start);
			 }, "name"_a, "start"_a)
		.def("sliceByNumber",
			 [](const kit::CustomizedCameraData &self, const std::string &name, const size_t start, const size_t length) {
				return self.sliceByNumber(name, start, length);
			 }, "name"_a, "start"_a, "length"_a)
		.def("sliceByNumber",
			 [](const kit::CustomizedCameraData &self, const std::string &name, const size_t start) {
				return self.sliceByNumber(name, start);
			 }, "name"_a, "start"_a)
		.def("sliceByNumber",
			 [](const kit::CustomizedCameraData &self, const std::string &name, const size_t start, const size_t length) {
				return self.sliceByNumber(name, start, length);
			 }, "name"_a, "start"_a, "length"_a)
		.def("sliceByTime",
			 [](const kit::CustomizedCameraData &self, const std::string &name, const int64_t start) {
				return self.sliceByTime(name, start);
			 }, "name"_a, "start"_a)
		.def("sliceByTime",
			 [](const kit::CustomizedCameraData &self, const std::string &name, const int64_t start, const int64_t end) {
				return self.sliceByTime(name, start, end);
			 }, "name"_a, "start"_a, "end"_a)
		.def("size", &kit::CustomizedCameraData::size)
		.def("timeWindow", &kit::CustomizedCameraData::timeWindow);

	py::class_<kit::MonoCameraSlicer>(m, "MonoCameraSlicer")
		.def(py::init<>())
		.def("accept", &kit::MonoCameraSlicer::accept)
		.def("doEveryNumberOfElements",
			 [](kit::MonoCameraSlicer &self, const std::string &name, 
			 	const size_t n, std::function<void(const kit::MonoCameraData &)> callback){
					return self.doEveryNumberOfElements(name, n, std::move(callback));
			 })
		.def("doEveryNumberOfElements",
			 [](kit::MonoCameraSlicer &self, const std::string &name, 
			 	const size_t n, std::function<void(const dv::TimeWindow &, const kit::MonoCameraData &)> callback){
					return self.doEveryNumberOfElements(name, n, std::move(callback));
			 })
		.def("doEveryTimeInterval",
			 [](kit::MonoCameraSlicer &self, const std::string &name, 
			 	const dv::Duration &interval, std::function<void(const kit::MonoCameraData &)> callback){
					return self.doEveryTimeInterval(name, interval, std::move(callback));
			 })
		.def("doEveryTimeInterval",
			 [](kit::MonoCameraSlicer &self, const std::string &name, 
			 	const dv::Duration &interval, std::function<void(const dv::TimeWindow &, const kit::MonoCameraData &)> callback){
					return self.doEveryTimeInterval(name, interval, std::move(callback));
			 })
		.def("hasJob", &kit::MonoCameraSlicer::hasJob)
		.def("removeJob", &kit::MonoCameraSlicer::removeJob)
		.def("modifyTimeInterval", &kit::MonoCameraSlicer::modifyTimeInterval)
		.def("modifyNumberInterval", &kit::MonoCameraSlicer::modifyNumberInterval);

	auto m_io = m.def_submodule("io");

	py::class_<kit::io::MonoCameraReader>(m_io, "MonoCameraReader")
		.def(py::init<const fs::path &>())
		.def("loadData", &kit::io::MonoCameraReader::loadData)
		.def("getResolution", &kit::io::MonoCameraReader::getResolution)
		.def("getEventResolution", &kit::io::MonoCameraReader::getEventResolution)
		.def("getFrameResolution", &kit::io::MonoCameraReader::getFrameResolution);

	py::class_<kit::io::MonoCameraWriter>(m_io, "MonoCameraWriter")
		.def(py::init<const fs::path &, const cv::Size &>())
		.def("writeData", &kit::io::MonoCameraWriter::writeData);

	auto m_simulation = m.def_submodule("simulation");

	m_simulation.def("generateSampleEvents", &kit::simulation::generateSampleEvents);	
}
