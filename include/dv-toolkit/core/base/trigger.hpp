#pragma once

#include "common.hpp"

namespace dv::toolkit {

struct TriggerPacket : public Packet<dv::Trigger> {
    typedef TriggerPacketFlatbuffer TableType;

	static FLATBUFFERS_CONSTEXPR const char *GetFullyQualifiedName() {
		return "dv.TriggerPacket";
	}
};

class TriggerStorage : public AddressableStorage<dv::Trigger, toolkit::TriggerPacket, toolkit::TriggerStorage> {
public:
	using AddressableStorage<dv::Trigger, toolkit::TriggerPacket, toolkit::TriggerStorage>::AddressableStorage;
};

} // namespace dv::toolkit
