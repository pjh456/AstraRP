#include <iostream>

#include "pipeline/event_bus.hpp"
#include "utils/error.hpp"

int main()
{
    using namespace astra_rp;

    pipeline::EventBus bus;

    int token_hits = 0;
    int token_hits_2 = 0;
    Str last_token_id;
    Str last_token_text;

    int state_hits = 0;
    Str last_state_id;
    pipeline::NodeState last_state = pipeline::NodeState::PENDING;

    int error_hits = 0;
    Str last_error_id;
    Str last_error_msg;

    bus.subscribe_token([&](const Str &id, const Str &text)
                        {
                            token_hits++;
                            last_token_id = id;
                            last_token_text = text;
                        });

    bus.subscribe_token([&](const Str &, const Str &)
                        { token_hits_2++; });

    bus.subscribe_state([&](const Str &id, pipeline::NodeState state)
                        {
                            state_hits++;
                            last_state_id = id;
                            last_state = state;
                        });

    bus.subscribe_error([&](const Str &id, utils::Error err)
                        {
                            error_hits++;
                            last_error_id = id;
                            last_error_msg = err.message();
                        });

    bus.publish_token("node_token", "hello");
    bus.publish_state("node_state", pipeline::NodeState::RUNNING);

    auto err = utils::ErrorBuilder()
                   .pipeline()
                   .node_execution_failed()
                   .message("boom")
                   .build();
    bus.publish_error("node_error", err);

    if (token_hits != 1 || token_hits_2 != 1)
    {
        std::cerr << "Expected 2 token subscribers to be called once each, got "
                  << token_hits << " and " << token_hits_2 << std::endl;
        return 1;
    }

    if (last_token_id != "node_token" || last_token_text != "hello")
    {
        std::cerr << "Token payload mismatch: id=" << last_token_id
                  << ", text=" << last_token_text << std::endl;
        return 1;
    }

    if (state_hits != 1 || last_state_id != "node_state" || last_state != pipeline::NodeState::RUNNING)
    {
        std::cerr << "State event mismatch." << std::endl;
        return 1;
    }

    if (error_hits != 1 || last_error_id != "node_error" || last_error_msg != "boom")
    {
        std::cerr << "Error event mismatch: id=" << last_error_id
                  << ", msg=" << last_error_msg << std::endl;
        return 1;
    }

    std::cout << "EventBus tests passed." << std::endl;
    return 0;
}
