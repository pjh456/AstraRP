// --- START OF FILE test_diamond_models.cpp ---
#include <iostream>
#include <string>
#include <cassert>

#define ASTRA_SIMPLE_LOG

#include "core/model_manager.hpp"
#include "infer/session.hpp"
#include "infer/engine.hpp"
#include "pipeline/graph.hpp"
#include "pipeline/inference_node.hpp"
#include "pipeline/scheduler.hpp"
#include "infer/generation_config.hpp"
#include "utils/logger.hpp"

using namespace astra_rp;
using namespace astra_rp::pipeline;

int main()
{
    std::cout << "========== STARTING DIAMOND DAG TEST ==========" << std::endl;

    const char *env_dir = std::getenv("LLAMA_MODEL_DIR");
    if (!env_dir)
        return 0;

    auto &mm = core::ModelManager::instance();
    auto model_res =
        mm.load(env_dir,
                core::ModelParamsBuilder("shared_model").build());
    assert(model_res.is_ok());
    auto model = model_res.unwrap();

    auto ctx_params =
        core::ContextParamsBuilder()
            .context_size(1024)
            .batch_size(512)
            .max_seqs(4)
            .build();

    auto sampler_res =
        core::SamplerChainBuilder()
            .greedy()
            .build();
    assert(sampler_res.is_ok());
    auto sampler = std::move(sampler_res.unwrap());

    auto s1_res = infer::Session::create(model, ctx_params, core::SamplerChain(sampler), 0);
    assert(s1_res.is_ok());
    auto s1 = s1_res.unwrap();

    auto s2_res = infer::Session::create(model, ctx_params, core::SamplerChain(sampler), 1);
    assert(s2_res.is_ok());
    auto s2 = s2_res.unwrap();

    auto s3_res = infer::Session::create(model, ctx_params, core::SamplerChain(sampler), 2);
    assert(s3_res.is_ok());
    auto s3 = s3_res.unwrap();

    auto s4_res = infer::Session::create(model, ctx_params, core::SamplerChain(sampler), 3);
    assert(s4_res.is_ok());
    auto s4 = s4_res.unwrap();

    auto graph = std::make_shared<Graph>();

    auto n1 = std::make_shared<InferenceNode>("N1", nullptr, s1);
    auto n2 = std::make_shared<InferenceNode>("N2", nullptr, s2);
    auto n3 = std::make_shared<InferenceNode>("N3", nullptr, s3);
    auto n4 = std::make_shared<InferenceNode>("N4", nullptr, s4);

    infer::GenerationConfig conf;
    conf.max_tokens = 60;

    n1->set_prompt_builder([](const auto &inputs)
                           { return "User: Name a random fruit. Just the name.\nAssistant:"; });
    n1->set_config(conf);

    n2->set_prompt_builder([](const auto &inputs)
                           { return "User: What is the typical color of " + inputs.at("N1").output + "?\nAssistant:"; });
    n2->set_config(conf);

    n3->set_prompt_builder([](const auto &inputs)
                           { return "User: In which climate does " + inputs.at("N1").output + " grow?\nAssistant:"; });
    n3->set_config(conf);

    n4->set_prompt_builder([](const auto &inputs)
                           { return "User: Combine this info into one sentence.\nColor: " +
                                    inputs.at("N2").output + "\nClimate: " +
                                    inputs.at("N3").output + "\nAssistant:"; });
    n4->set_config(conf);

    graph->add_node(n1);
    graph->add_node(n2);
    graph->add_node(n3);
    graph->add_node(n4);

    graph->add_edge("N1", "N2");
    graph->add_edge("N1", "N3");
    graph->add_edge("N2", "N4");
    graph->add_edge("N3", "N4");

    // Scheduler执行
    Scheduler scheduler(graph, 4);
    auto res = scheduler.run();
    assert(res.is_ok());

    ASTRA_LOG_INFO("=== DAG Execution Finished ===");
    ASTRA_LOG_INFO("N1 (Fruit): " + n1->output().output);
    ASTRA_LOG_INFO("N2 (Color): " + n2->output().output);
    ASTRA_LOG_INFO("N3 (Climate): " + n3->output().output);
    ASTRA_LOG_INFO("N4 (Summary): " + n4->output().output);

    return 0;
}