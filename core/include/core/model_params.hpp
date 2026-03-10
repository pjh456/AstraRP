#ifndef INCLUDE_ASTRA_RP_MODEL_PARAMS_HPP
#define INCLUDE_ASTRA_RP_MODEL_PARAMS_HPP

#include "utils/types.hpp"

#include "llama.h"

namespace astra_rp
{
    namespace core
    {
        class ModelManager;
        class ModelParams;
        class ModelParamsBuilder;
        class ModelParamsMetaData;

        struct ModelParamsMetaData
        {
            int32_t gpu_layers = 0;
            bool use_mlock = false;
            bool use_mmap = false;
            bool vocal_only = false;
        };

        class ModelParams
        {
            friend class ModelManager;
            friend class ModelParamsBuilder;

        private:
            llama_model_params m_params;
            Str m_name;

            ModelParamsMetaData m_metadata;

        public:
            ModelParams(const Str &name, llama_model_params params)
                : m_name(name), m_params(params) {}

            ModelParams(const Str &name)
                : ModelParams(name, llama_model_default_params()) {}

            ~ModelParams() = default;

            ModelParams(const ModelParams &) = default;
            ModelParams &operator=(const ModelParams &) = default;

            ModelParams(ModelParams &&) noexcept = default;
            ModelParams &operator=(ModelParams &&) noexcept = default;

        public:
            llama_model_params raw() const noexcept { return m_params; }
            ModelParamsMetaData metadata() const noexcept { return m_metadata; }
        };

        class ModelParamsBuilder
        {
        private:
            ModelParams m_params;

        public:
            ModelParamsBuilder(const Str &name) : m_params(name) {}

            ModelParams build() const { return m_params; }

        public:
            ModelParamsBuilder &gpu_layers(int32_t n_layers)
            {
                m_params.m_params.n_gpu_layers = n_layers;
                m_params.m_metadata.gpu_layers = n_layers;
                return *this;
            }

            ModelParamsBuilder &use_mmap(bool flag)
            {
                m_params.m_params.use_mmap = flag;
                m_params.m_metadata.use_mmap = flag;
                return *this;
            }

            ModelParamsBuilder &use_mlock(bool flag)
            {
                m_params.m_params.use_mlock = flag;
                m_params.m_metadata.use_mlock = flag;
                return *this;
            }

            ModelParamsBuilder &vocal_only(bool flag)
            {
                m_params.m_params.vocab_only = flag;
                m_params.m_metadata.vocal_only = flag;
                return *this;
            }
        };
    }
}

#endif // INCLUDE_ASTRA_RP_MODEL_PARAMS_HPP