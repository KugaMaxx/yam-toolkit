#pragma once

#include "common.hpp"

namespace dv::toolkit {

struct IMUPacket : public Packet<dv::IMU> {
    typedef IMUPacketFlatbuffer TableType;

	static FLATBUFFERS_CONSTEXPR const char *GetFullyQualifiedName() {
		return "dv.IMUPacket";
	}
};

class IMUStorage : public AddressableStorage<dv::IMU, toolkit::IMUPacket, toolkit::IMUStorage> {
public:
	using AddressableStorage<dv::IMU, toolkit::IMUPacket, toolkit::IMUStorage>::AddressableStorage;
};

} // namespace dv::toolkit
