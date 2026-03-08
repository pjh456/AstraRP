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

    // 1. 启动全局推理引擎
    infer::Engine::instance().start();

    // 2. 加载模型 (为了节省内存，4个节点共享同一个物理模型，但上下文Session独立)
    const char *env_dir = std::getenv("LLAMA_MODEL_DIR");
    if (!env_dir)
        return 0;

    auto &mm = core::ModelManager::instance();
    auto model_res =
        mm.load(env_dir,
                core::ModelParamsBuilder("shared_model").build());
    assert(model_res.is_ok());
    auto model = model_res.unwrap();

    // 3. 为 4 个节点创建 4 个独立的 Session (它们各自拥有独立的 KV Cache)
    auto ctx_params =
        core::ContextParamsBuilder()
            .context_size(1024)
            .batch_size(512)
            .max_seqs(4)
            .build();

    auto sampler_res =
        core::SamplerBuilder()
            .greedy()
            .build();
    assert(sampler_res.is_ok());
    auto sampler = sampler_res.unwrap();

    auto s1_res = infer::Session::create(model, ctx_params, core::Sampler(sampler), 0);
    assert(s1_res.is_ok());
    auto s1 = s1_res.unwrap();

    auto s2_res = infer::Session::create(model, ctx_params, core::Sampler(sampler), 1);
    assert(s2_res.is_ok());
    auto s2 = s2_res.unwrap();

    auto s3_res = infer::Session::create(model, ctx_params, core::Sampler(sampler), 2);
    assert(s3_res.is_ok());
    auto s3 = s3_res.unwrap();

    auto s4_res = infer::Session::create(model, ctx_params, core::Sampler(sampler), 3);
    assert(s4_res.is_ok());
    auto s4 = s4_res.unwrap();

    // 4. 构建 DAG 图与节点
    auto graph = std::make_shared<Graph>();

    auto n1 = std::make_shared<InferenceNode>("N1", nullptr, s1);
    auto n2 = std::make_shared<InferenceNode>("N2", nullptr, s2);
    auto n3 = std::make_shared<InferenceNode>("N3", nullptr, s3);
    auto n4 = std::make_shared<InferenceNode>("N4", nullptr, s4);

    // --- 核心：配置 Prompt 拼装逻辑 ---
    infer::GenerationConfig conf;
    conf.max_tokens = 60;

    // N1 独立起点
    n1->set_prompt_builder(
        [](const auto &inputs)
        {
            return "User: Name a random fruit. Just the name.\nAssistant:";
        });
    n1->set_config(conf);

    // N2 接收 N1 的输出
    n2->set_prompt_builder(
        [](const auto &inputs)
        {
            Str n1_fruit = inputs.at("N1").output; // 获取 N1 的结果
            return "User: What is the typical color of " + n1_fruit + "?\nAssistant:";
        });
    n2->set_config(conf);

    // N3 接收 N1 的输出
    n3->set_prompt_builder(
        [](const auto &inputs)
        {
            Str n1_fruit = inputs.at("N1").output;
            return "User: In which climate does " + n1_fruit + " grow?\nAssistant:";
        });
    n3->set_config(conf);

    // N4 接收 N2 和 N3 的输出（菱形汇合！）
    n4->set_prompt_builder(
        [](const auto &inputs)
        {
            Str color = inputs.at("N2").output;
            Str climate = inputs.at("N3").output;
            return "User: Combine this info into one sentence.\nColor: " + color + "\nClimate: " + climate + "\nAssistant:";
        });
    n4->set_config(conf);

    // 5. 编排拓扑边
    graph->add_node(n1);
    graph->add_node(n2);
    graph->add_node(n3);
    graph->add_node(n4);

    graph->add_edge("N1", "N2");
    graph->add_edge("N1", "N3");
    graph->add_edge("N2", "N4");
    graph->add_edge("N3", "N4");

    // 6. 执行调度 (Scheduler内部的 Worker 线程会自动并发执行 N2 和 N3)
    Scheduler scheduler(graph, 4); // 开启 4 个并发线程
    auto res = scheduler.run();
    assert(res.is_ok());

    // 7. 打印最终结果
    ASTRA_LOG_INFO("=== DAG Execution Finished ===");
    ASTRA_LOG_INFO("N1 (Fruit): " + n1->output().output);
    ASTRA_LOG_INFO("N2 (Color): " + n2->output().output);
    ASTRA_LOG_INFO("N3 (Climate): " + n3->output().output);
    ASTRA_LOG_INFO("N4 (Summary): " + n4->output().output);

    infer::Engine::instance().stop();
    return 0;
}