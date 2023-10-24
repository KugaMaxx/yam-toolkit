#pragma once

#include "common.hpp"

namespace dv::toolkit {

struct FramePacketFlatbuffer;

struct FramePacket : public Packet<dv::Frame> {
    typedef FramePacketFlatbuffer TableType;

	static FLATBUFFERS_CONSTEXPR const char *GetFullyQualifiedName() {
		return "dv.FramePacket";
	}
};

class FrameStorage : public AddressableStorage<dv::Frame, toolkit::FramePacket, toolkit::FrameStorage> {
public:
	using AddressableStorage<dv::Frame, toolkit::FramePacket, toolkit::FrameStorage>::AddressableStorage;
};

} // namespace dv::toolkit
