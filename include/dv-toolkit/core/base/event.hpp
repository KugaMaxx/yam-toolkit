#pragma once

#include "common.hpp"

#include <dv-processing/core/core.hpp>

namespace dv::toolkit {

struct EventPacket : public Packet<dv::Event> {
    typedef EventPacketFlatbuffer TableType;

	static FLATBUFFERS_CONSTEXPR const char *GetFullyQualifiedName() {
		return "dv.EventPacket";
	}
};

class EventStorage : public AddressableStorage<dv::Event, toolkit::EventPacket, toolkit::EventStorage> {
public:
	using AddressableEventStorage = AddressableStorage<dv::Event, toolkit::EventPacket, toolkit::EventStorage>;
	using AddressableEventStorage::AddressableStorage;

	[[nodiscard]] EventPacket toPacket() const {
		EventPacket packet;
		packet.elements.reserve(size());
		for (const auto &element : *this) {
			packet.elements.push_back(dv::Event(element.timestamp(), element.x(), element.y(), element.polarity()));
		}
		return packet;
	}

	[[nodiscard]] Eigen::Matrix<int64_t, Eigen::Dynamic, 1> timestamps() const {
		Eigen::Matrix<int64_t, Eigen::Dynamic, 1> timestamps(size());
		int64_t *target = timestamps.data();
		for (const auto &event : *this) {
			*target = event.timestamp();
			target++;
		}
		return timestamps;
	}

	[[nodiscard]] Eigen::Matrix<int16_t, Eigen::Dynamic, 1> xs() const {
		Eigen::Matrix<int16_t, Eigen::Dynamic, 1> abscissae(size(), 1);
		int16_t *x = abscissae.data();
		for (const auto &event : *this) {
			*x = event.x();
			x++;
		}
		return abscissae;
	}

	[[nodiscard]] Eigen::Matrix<int16_t, Eigen::Dynamic, 1> ys() const {
		Eigen::Matrix<int16_t, Eigen::Dynamic, 1> ordinates(size(), 1);
		int16_t *y = ordinates.data();
		for (const auto &event : *this) {
			*y = event.y();
			y++;
		}
		return ordinates;
	}

	[[nodiscard]] Eigen::Matrix<int16_t, Eigen::Dynamic, 2> coordinates() const {
		Eigen::Matrix<int16_t, Eigen::Dynamic, 2> coordinates(size(), 2);
		int16_t *x = coordinates.data();
		int16_t *y = coordinates.data() + size();
		for (const auto &event : *this) {
			*x = event.x();
			x++;
			*y = event.y();
			y++;
		}
		return coordinates;
	}

	[[nodiscard]] Eigen::Matrix<uint8_t, Eigen::Dynamic, 1> polarities() const {
		Eigen::Matrix<uint8_t, Eigen::Dynamic, 1> polarities(size());
		uint8_t *target = polarities.data();
		for (const auto &event : *this) {
			*target = event.polarity();
			target++;
		}
		return polarities;
	}

	dv::EventStore toEventStore() const {
		dv::EventStore store;
		for (const auto &event : *this) {
			store.emplace_back(event.timestamp(), event.x(), event.y(), event.polarity());
		}
		return store;
	}
};

} // namespace dv::toolkit