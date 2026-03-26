#include <iostream>
#include <atomic>

#include "pipeline/base_node.hpp"
#include "pipeline/event_bus.hpp"

namespace astra_rp
{
    namespace pipeline
    {
        class StateNode final : public BaseNode
        {
        public:
            StateNode(const Str &id, MulPtr<EventBus> bus)
                : BaseNode(id, bus) {}

            ResultV<void> execute() override
            {
                return ResultV<void>::Ok();
            }

            void trigger_state(NodeState state)
            {
                update_state(state);
            }
        };
    }
}

int main()
{
    using namespace astra_rp;

    auto bus = std::make_shared<pipeline::EventBus>();

    std::atomic<int> hits{0};
    Str last_id;
    pipeline::NodeState last_state = pipeline::NodeState::PENDING;

    bus->subscribe_state([&](const Str &id, pipeline::NodeState state)
                         {
                             hits.fetch_add(1);
                             last_id = id;
                             last_state = state;
                         });

    pipeline::StateNode node("N1", bus);
    node.trigger_state(pipeline::NodeState::RUNNING);

    if (hits.load() != 1)
    {
        std::cerr << "Expected state subscriber to be called." << std::endl;
        return 1;
    }

    if (last_id != "N1" || last_state != pipeline::NodeState::RUNNING)
    {
        std::cerr << "State event payload mismatch." << std::endl;
        return 1;
    }

    std::cout << "BaseNode state event test passed." << std::endl;
    return 0;
}
