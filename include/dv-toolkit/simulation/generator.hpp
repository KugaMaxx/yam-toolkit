#pragma once

#include "../core/core.hpp"
#include <random>

namespace dv::toolkit::simulation {

[[nodiscard]] inline dv::toolkit::EventStorage generateSampleEvents(const cv::Size &resolution) {
	std::default_random_engine generator(0);
	std::uniform_real_distribution<double> distribution(0.0, 1.0);

	dv::toolkit::EventStorage store;
	for (int64_t i = 0; i < 1000; i++) {
		store.emplace_back(
            dv::now(),
			static_cast<int16_t>(distribution(generator) * resolution.width),
			static_cast<int16_t>(distribution(generator) * resolution.height), 
            distribution(generator) > 0.5);
	}
	return store;
}

} // namespace dv::toolkit::simulation
