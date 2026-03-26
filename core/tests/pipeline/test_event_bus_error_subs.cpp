#include <iostream>
#include <atomic>

#include "pipeline/event_bus.hpp"
#include "utils/error.hpp"

int main()
{
    using namespace astra_rp;

    pipeline::EventBus bus;
    std::atomic<int> hits{0};

    bus.subscribe_error([&](const Str &, utils::Error)
                        { hits.fetch_add(1); });
    bus.subscribe_error([&](const Str &, utils::Error)
                        { hits.fetch_add(1); });

    auto err = utils::ErrorBuilder()
                   .pipeline()
                   .node_execution_failed()
                   .message("boom")
                   .build();
    bus.publish_error("node", err);

    if (hits.load() != 2)
    {
        std::cerr << "Expected two error subscribers to fire." << std::endl;
        return 1;
    }

    std::cout << "EventBus error subscriber test passed." << std::endl;
    return 0;
}
