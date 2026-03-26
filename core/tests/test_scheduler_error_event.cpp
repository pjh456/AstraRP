#include <iostream>
#include <mutex>
#include <atomic>

#include "pipeline/graph.hpp"
#include "pipeline/base_node.hpp"
#include "pipeline/scheduler.hpp"
#include "pipeline/event_bus.hpp"
#include "utils/error.hpp"

namespace astra_rp
{
    namespace pipeline
    {
        class FailingNode final : public BaseNode
        {
        public:
            FailingNode(const Str &id, MulPtr<EventBus> bus = nullptr)
                : BaseNode(id, bus) {}

            ResultV<void> execute() override
            {
                return ResultV<void>::Err(
                    utils::ErrorBuilder()
                        .pipeline()
                        .node_execution_failed()
                        .message("boom")
                        .build());
            }
        };
    }
}

int main()
{
    using namespace astra_rp;

    auto bus = std::make_shared<pipeline::EventBus>();

    std::atomic<bool> error_called{false};
    std::mutex mtx;
    Str error_id;
    Str error_message;
    bool has_cause = false;

    bus->subscribe_error([&](const Str &id, utils::Error err)
                         {
                             error_called.store(true);
                             std::lock_guard<std::mutex> lock(mtx);
                             error_id = id;
                             error_message = err.message();
                             has_cause = (err.cause() != nullptr);
                         });

    auto graph = std::make_shared<pipeline::Graph>();
    graph->add_node(std::make_shared<pipeline::FailingNode>("A"));

    pipeline::Scheduler scheduler(graph, 1, bus);
    auto res = scheduler.run();

    if (res.is_ok())
    {
        std::cerr << "Expected scheduler to fail, but it succeeded." << std::endl;
        return 1;
    }

    if (!error_called.load())
    {
        std::cerr << "Expected error event to be published." << std::endl;
        return 1;
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        if (error_id != "SCHEDULER")
        {
            std::cerr << "Unexpected error id: " << error_id << std::endl;
            return 1;
        }
        if (error_message.find("Pipeline halted due to node failure") == Str::npos)
        {
            std::cerr << "Unexpected error message: " << error_message << std::endl;
            return 1;
        }
        if (!has_cause)
        {
            std::cerr << "Expected error to wrap cause." << std::endl;
            return 1;
        }
    }

    std::cout << "Scheduler error event test passed." << std::endl;
    return 0;
}
