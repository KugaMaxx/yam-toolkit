#include <dv-toolkit/core/core.hpp>

int main() {
    namespace kit = dv::toolkit;

    // Initialize MonoCameraData
    kit::MonoCameraData data;

    // Access immutable variables through functions, support types are:
    // events, frames, imus and triggers
    std::cout << data.events() << std::endl;
    // "Storage is empty!"

    // Emplace back event elements, the function arguments are:
    // timestamp, x, y, polarity
    kit::EventStorage store;
    store.emplace_back(dv::now(), 0, 0, false);
    store.emplace_back(dv::now(), 1, 1, true);
    store.emplace_back(dv::now(), 2, 2, false);
    store.emplace_back(dv::now(), 3, 3, true);

    // Assignment value to MonoCameraData
    data["events"] = store;

    // Access mutable variables through std::get
    std::cout << std::get<kit::EVTS>(data["events"]) << std::endl;
    // "Storage containing 4 elements within ..."

    return 0;
}