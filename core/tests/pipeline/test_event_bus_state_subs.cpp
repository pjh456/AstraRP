#include <iostream>
#include <atomic>

#include "pipeline/event_bus.hpp"

int main()
{
    using namespace astra_rp;

    pipeline::EventBus bus;
    std::atomic<int> hits{0};

    bus.subscribe_state([&](const Str &, pipeline::NodeState)
                        { hits.fetch_add(1); });
    bus.subscribe_state([&](const Str &, pipeline::NodeState)
                        { hits.fetch_add(1); });

    bus.publish_state("N", pipeline::NodeState::READY);

    if (hits.load() != 2)
    {
        std::cerr << "Expected two state subscribers to fire." << std::endl;
        return 1;
    }

    std::cout << "EventBus state subscriber test passed." << std::endl;
    return 0;
}
