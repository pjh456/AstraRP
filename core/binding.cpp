#include <napi.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <memory>
#include <vector>

#include "core/global_config.hpp"
#include "core/tokenizer.hpp"
#include "core/model_manager.hpp"
#include "pipeline/graph.hpp"
#include "pipeline/scheduler.hpp"
#include "pipeline/event_bus.hpp"
#include "pipeline/inference_node.hpp"
#include "pipeline/format_node.hpp"
#include "pipeline/output_node.hpp"
#include "utils/logger.hpp"

using namespace astra_rp;

// 1. 暴露全局初始化方法
Napi::Value InitSystem(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString())
    {
        Napi::TypeError::New(
            env,
            "String expected for config path")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string configPath =
        info[0].As<Napi::String>().Utf8Value();
    auto res =
        core::GlobalConfigManager::instance()
            .load_from_file(configPath);

    if (res.is_err())
    {
        Napi::Error::New(
            env,
            res.unwrap_err().to_string())
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    return Napi::Boolean::New(env, true);
}

Napi::ThreadSafeFunction g_log_tsfn;

Napi::Value SetLogCallback(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsFunction())
    {
        Napi::TypeError::
            New(env, "Function expected")
                .ThrowAsJavaScriptException();
        return env.Null();
    }

    g_log_tsfn =
        Napi::ThreadSafeFunction::New(
            env,
            info[0].As<Napi::Function>(), "LogCallback", 0, 1);
    g_log_tsfn.Unref(env); // 避免由于日志监听导致 Node 进程无法自然退出

    astra_rp::utils::Logger::instance().set_callback(
        [](astra_rp::utils::LogLevel level,
           const astra_rp::Str &file,
           int line,
           const astra_rp::Str &msg)
        {
            if (!g_log_tsfn)
                return;

            struct LogData
            {
                int level;
                std::string file;
                int line;
                std::string msg;
            };
            auto *data =
                new LogData{
                    static_cast<int>(level),
                    file,
                    line,
                    msg};

            // 使用非阻塞调用防止 C++ 推理主线程被锁死
            g_log_tsfn.NonBlockingCall(
                data,
                [](Napi::Env env,
                   Napi::Function jsCb,
                   LogData *data)
                {
                    if (env != nullptr && jsCb != nullptr)
                    {
                        jsCb.Call(
                            {Napi::Number::New(env, data->level),
                             Napi::String::New(env, data->file),
                             Napi::Number::New(env, data->line),
                             Napi::String::New(env, data->msg)});
                    }
                    delete data;
                });
        });
    return env.Undefined();
}

Napi::Value Detokenize(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    // 参数1：模型名称 (比如 "default")
    // 参数2：Token 数组 [123, 456, 789]
    if (info.Length() < 2 || !info[1].IsArray())
    {
        Napi::TypeError::New(env, "Expected model_name and token array").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Array token_array = info[1].As<Napi::Array>();
    std::vector<int32_t> tokens;
    for (uint32_t i = 0; i < token_array.Length(); ++i)
    {
        tokens.push_back(token_array.Get(i).As<Napi::Number>().Int32Value());
    }

    // 获取当前模型 (基于你的框架，这里复用 config 模型获取)
    auto modelRes = astra_rp::core::ModelManager::instance().load_config_model();
    if (modelRes.is_err())
    {
        Napi::Error::New(env, "Model not loaded").ThrowAsJavaScriptException();
        return env.Null();
    }

    // 调用引擎层的 Detokenize
    auto strRes = astra_rp::core::Tokenizer::detokenize(modelRes.unwrap(), tokens);
    if (strRes.is_err())
    {
        Napi::Error::New(env, strRes.unwrap_err().to_string()).ThrowAsJavaScriptException();
        return env.Null();
    }

    return Napi::String::New(env, strRes.unwrap());
}

// 2. 异步 Worker：用于在后台运行 Scheduler，不阻塞 Node 主线程
class PipelineRunWorker : public Napi::AsyncWorker
{
private:
    std::shared_ptr<pipeline::Scheduler> m_scheduler;
    utils::Result<void, utils::Error>
        m_result =
            utils::Result<void, utils::Error>::Ok();

public:
    PipelineRunWorker(
        Napi::Function &callback,
        std::shared_ptr<pipeline::Scheduler> scheduler)
        : Napi::AsyncWorker(callback), m_scheduler(scheduler) {}

    ~PipelineRunWorker() {}

    // 在 C++ 后台线程中执行
    void Execute() override
    {
        m_result = m_scheduler->run();
    }

    // 在 Node 主线程中回调
    void OnOK() override
    {
        Napi::HandleScope scope(Env());
        if (m_result.is_err())
        {
            Callback().Call({Napi::Error::New(Env(), m_result.unwrap_err().to_string()).Value()});
        }
        else
        {
            Callback().Call({Env().Null(), Napi::String::New(Env(), "Success")});
        }
    }
};

// 3. 包装 Pipeline/DAG 供 JS 调用
class PipelineWrapper : public Napi::ObjectWrap<PipelineWrapper>
{
private:
    std::shared_ptr<pipeline::Graph> m_graph;
    std::shared_ptr<pipeline::EventBus> m_bus;

    // 保存 ThreadSafeFunction 以防止被垃圾回收
    Napi::ThreadSafeFunction m_tsfn_token;

    // [新增] 用来保存 Output 节点的引用，方便后续直接查数据和清空
    std::unordered_map<
        std::string,
        std::shared_ptr<pipeline::OutputNode>>
        m_output_nodes;

public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports)
    {
        Napi::Function func =
            DefineClass(
                env,
                "Pipeline",
                {
                    InstanceMethod(
                        "addFormatNode",
                        &PipelineWrapper::AddFormatNode),
                    InstanceMethod(
                        "addInferenceNode",
                        &PipelineWrapper::AddInferenceNode),
                    InstanceMethod(
                        "addOutputNode",
                        &PipelineWrapper::AddOutputNode),
                    InstanceMethod(
                        "getOutputContent",
                        &PipelineWrapper::GetOutputContent),
                    InstanceMethod(
                        "clearOutput",
                        &PipelineWrapper::ClearOutput),
                    InstanceMethod(
                        "addEdge",
                        &PipelineWrapper::AddEdge),
                    InstanceMethod(
                        "onToken",
                        &PipelineWrapper::OnToken),
                    InstanceMethod(
                        "run",
                        &PipelineWrapper::Run),
                });

        Napi::FunctionReference *constructor = new Napi::FunctionReference();
        *constructor = Napi::Persistent(func);
        env.SetInstanceData(constructor);

        exports.Set("Pipeline", func);
        return exports;
    }

    PipelineWrapper(const Napi::CallbackInfo &info)
        : Napi::ObjectWrap<PipelineWrapper>(info)
    {
        m_graph = std::make_shared<pipeline::Graph>();
        m_bus = std::make_shared<pipeline::EventBus>();
    }

    // 添加格式化节点 (简化演示，实际可传入 JS 回调作为 Formatter)
    Napi::Value AddFormatNode(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();
        std::string id = info[0].As<Napi::String>().Utf8Value();
        std::string format_spec = info[1].As<Napi::String>().Utf8Value();

        Vec<pipeline::FormatNode::FormatPart> parts;
        try
        {
            auto json = nlohmann::json::parse(format_spec);
            if (json.is_array())
            {
                for (const auto &item : json)
                {
                    if (!item.is_object())
                        continue;

                    const auto type = item.value("type", "text");
                    if (type == "node")
                    {
                        parts.push_back({
                            pipeline::FormatNode::FormatPart::Type::NODE,
                            item.value("id", Str())});
                    }
                    else
                    {
                        parts.push_back({
                            pipeline::FormatNode::FormatPart::Type::TEXT,
                            item.value("value", Str())});
                    }
                }
            }
        }
        catch (...)
        {
            parts.push_back({
                pipeline::FormatNode::FormatPart::Type::TEXT,
                format_spec});
        }

        if (parts.empty())
        {
            parts.push_back({
                pipeline::FormatNode::FormatPart::Type::TEXT,
                format_spec});
        }

        auto formatter =
            [parts = std::move(parts)](const HashMap<Str, pipeline::NodePayload> &inputs) -> Str
        {
            return pipeline::FormatNode::apply_format(parts, inputs);
        };

        auto node = std::make_shared<pipeline::FormatNode>(id, m_bus, formatter);
        m_graph->add_node(node);

        return env.Undefined();
    }

    // 添加推理节点
    Napi::Value AddInferenceNode(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();
        std::string id = info[0].As<Napi::String>().Utf8Value();

        // 使用你框架中的默认构造
        infer::TokenizeParams tp;
        infer::DecodeParams dp;
        dp.max_tokens = 100;
        infer::SampleParams sp;

        auto nodeRes = pipeline::InferenceNode::default_create(id, tp, dp, sp, m_bus);
        if (nodeRes.is_err())
        {
            Napi::Error::New(env, nodeRes.unwrap_err().to_string()).ThrowAsJavaScriptException();
            return env.Undefined();
        }

        auto infer_node = std::move(nodeRes.unwrap());
        infer_node.set_prompt_builder(
            [](const HashMap<Str, pipeline::NodePayload> &inputs) -> Str
            {
                if (inputs.empty())
                    return ""; // 兜底防止空指针
                return inputs.begin()->second.output;
            });
        auto node = std::make_shared<pipeline::InferenceNode>(infer_node);
        m_graph->add_node(node);

        return env.Undefined();
    }

    // 添加 Output 节点
    Napi::Value AddOutputNode(const Napi::CallbackInfo &info)
    {
        std::string id = info[0].As<Napi::String>().Utf8Value();
        auto node = std::make_shared<pipeline::OutputNode>(id, m_bus);
        m_graph->add_node(node);
        m_output_nodes[id] = node; // 记录引用
        return info.Env().Undefined();
    }

    // 获取内容
    Napi::Value GetOutputContent(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();
        std::string id = info[0].As<Napi::String>().Utf8Value();
        if (m_output_nodes.count(id))
        {
            return Napi::String::New(env, m_output_nodes[id]->content());
        }
        return Napi::String::New(env, "");
    }

    // 清空内容
    Napi::Value ClearOutput(const Napi::CallbackInfo &info)
    {
        std::string id = info[0].As<Napi::String>().Utf8Value();
        if (m_output_nodes.count(id))
        {
            m_output_nodes[id]->clear();
        }
        return info.Env().Undefined();
    }

    // 添加边
    Napi::Value AddEdge(const Napi::CallbackInfo &info)
    {
        std::string from = info[0].As<Napi::String>().Utf8Value();
        std::string to = info[1].As<Napi::String>().Utf8Value();
        m_graph->add_edge(from, to);
        return info.Env().Undefined();
    }

    // 绑定 Token 流式输出回调
    Napi::Value OnToken(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();
        Napi::Function jsCallback = info[0].As<Napi::Function>();

        // 创建线程安全的函数，因为 Scheduler 的 run 是在 C++ 子线程中调用回调的
        m_tsfn_token = Napi::ThreadSafeFunction::New(
            env, jsCallback, "TokenCallback", 0, 1);

        // 订阅 C++ 的 EventBus
        m_bus->subscribe_token([this](const Str &id, const Str &text)
                               {
            // 包装参数传给 JS
            auto callback =
            [](Napi::Env env, Napi::Function jsCb, std::pair<Str, Str>* data) {
                jsCb.Call({ Napi::String::New(env, data->first), Napi::String::New(env, data->second) });
                delete data;
            };
            // 将数据发往 Node 主线程执行
            auto* data = new std::pair<Str, Str>(id, text);
            m_tsfn_token.BlockingCall(data, callback); });

        return env.Undefined();
    }

    // 执行流水线
    Napi::Value Run(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();
        Napi::Function jsCallback = info[0].As<Napi::Function>();

        // 验证图
        auto validRes = m_graph->validate();
        if (validRes.is_err())
        {
            Napi::Error::New(env, validRes.unwrap_err().to_string()).ThrowAsJavaScriptException();
            return env.Undefined();
        }

        // 创建 Scheduler 并通过 AsyncWorker 在后台执行
        auto scheduler = std::make_shared<pipeline::Scheduler>(m_graph, 4, m_bus);
        PipelineRunWorker *worker = new PipelineRunWorker(jsCallback, scheduler);
        worker->Queue();

        return env.Undefined();
    }
};

// 4. 导出模块
Napi::Object InitAll(Napi::Env env, Napi::Object exports)
{
    exports.Set("initSystem", Napi::Function::New(env, InitSystem));
    exports.Set("setLogCallback", Napi::Function::New(env, SetLogCallback));
    exports.Set("detokenize", Napi::Function::New(env, Detokenize));
    return PipelineWrapper::Init(env, exports);
}

NODE_API_MODULE(astra_rp, InitAll)