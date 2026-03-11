#include "core/global_config.hpp"

#include <fstream>
#include <sstream>

#include "pjh_json/parsers/json_parser.hpp"

#define METHOD_CONCAT_HELPER(builder, method) builder.method
#define DESERIALIZE_HELPER(raw_data, ref, builder, method, convert_type) \
    if (raw_data->contains(#method))                                     \
    builder.method(ref[#method].convert_type())
#define EASY_DESERIALIZE(ref, method, type) DESERIALIZE_HELPER(ref##_data, ref, builder, method, as_##type)

#define JSON_KEY_VAL(stream, key, value) stream << "\"" << #key << "\":" << value
#define EASY_SERIALIZE_INT_END(method) JSON_KEY_VAL(file, method, meta.method)
#define EASY_SERIALIZE_INT(method) EASY_SERIALIZE_INT_END(method) << ","
#define EASY_SERIALIZE_BOOL_END(method) JSON_KEY_VAL(file, method, (meta.method ? "true" : "false"))
#define EASY_SERIALIZE_BOOL(method) EASY_SERIALIZE_BOOL_END(method) << ","

#define PARAM_CHECKER(obj, name, type) \
    if (!obj->contains(#name))         \
    return type(utils::ErrorBuilder().core().config_load_failed().message(#name "not find in config").build())

namespace astra_rp
{
    namespace core
    {
        ResultV<void>
        GlobalConfigManager::load_from_file(const Str &path)
        {
            using namespace pjh_std::json;

            std::lock_guard lock(m_mtx);

            std::ifstream file(path);
            if (!file)
            {
                return ResultV<void>::Err(
                    utils::ErrorBuilder()
                        .core()
                        .config_load_failed()
                        .message("Config file load failed!")
                        .build());
            }
            std::stringstream ss;
            ss << file.rdbuf();
            std::string config = ss.str();
            file.close();

            Parser parser(config);

            auto json = parser.parse();
            auto json_obj = json.get()->as_object();

            PARAM_CHECKER(json_obj, model_dir, ResultV<void>);
            m_data.model_dir = json["model_dir"].as_str();

            PARAM_CHECKER(json_obj, model_params, ResultV<void>);
            {
                auto mp = json["model_params"];
                auto mp_data = mp.get()->as_object();
                auto builder = ModelParamsBuilder("default");

                EASY_DESERIALIZE(mp, gpu_layers, int);
                EASY_DESERIALIZE(mp, use_mlock, bool);
                EASY_DESERIALIZE(mp, use_mmap, bool);

                m_data.model_params = builder.build();
            }

            PARAM_CHECKER(json_obj, context_params, ResultV<void>);
            {
                auto cp = json["context_params"];
                auto cp_data = cp.get()->as_object();
                auto builder = ContextParamsBuilder();

                EASY_DESERIALIZE(cp, context_size, int);
                EASY_DESERIALIZE(cp, batch_size, int);
                EASY_DESERIALIZE(cp, max_seqs, int);
                EASY_DESERIALIZE(cp, threads, int);
                EASY_DESERIALIZE(cp, threads_batch, int);
                EASY_DESERIALIZE(cp, flash_attention, bool);

                m_data.context_params = builder.build();
            }

            m_data.max_concurrency =
                json_obj->contains("max_concurrency")
                    ? json["max_concurrency"].as_int()
                    : 1;

            delete json.get();

            m_loaded = true;

            return ResultV<void>::Ok();
        }

        ResultV<void>
        GlobalConfigManager::save_to_file(const Str &path)
        {
            std::lock_guard lock(m_mtx);

            std::ofstream file(path);
            if (!file)
            {
                return ResultV<void>::Err(
                    utils::ErrorBuilder()
                        .core()
                        .config_load_failed()
                        .message("Config file create failed!")
                        .build());
            }

            file << "{";
            file << "\"model_dir\":" << "\"" << m_data.model_dir << "\",";
            {
                auto meta = m_data.model_params.metadata();
                file << "\"model_params\":{";
                EASY_SERIALIZE_INT(gpu_layers);
                EASY_SERIALIZE_BOOL(use_mlock);
                EASY_SERIALIZE_BOOL_END(use_mmap);
                file << "},";
            }

            {
                auto meta = m_data.context_params.metadata();
                file << "\"context_params\":{";
                EASY_SERIALIZE_INT(context_size);
                EASY_SERIALIZE_INT(batch_size);
                EASY_SERIALIZE_INT(max_seqs);
                EASY_SERIALIZE_INT(threads);
                EASY_SERIALIZE_INT(threads_batch);
                EASY_SERIALIZE_BOOL_END(flash_attention);
                file << "},";
            }

            file << "\"max_concurrency\":" << m_data.max_concurrency;
            file << "}";

            file.close();

            return ResultV<void>::Ok();
        }
    }
}