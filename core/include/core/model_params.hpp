#ifndef INCLUDE_ASTRA_RP_MODEL_PARAMS_HPP
#define INCLUDE_ASTRA_RP_MODEL_PARAMS_HPP

#include "utils/types.hpp"

#include "llama.h"

namespace astra_rp
{
    namespace core
    {
        class ModelManager;

        class ModelParams
        {
            friend class ModelManager;

        private:
            llama_model_params m_params;
            Str m_name;

        public:
            ModelParams(const Str &name, llama_model_params params);
            ModelParams(const Str &name);

            ~ModelParams() = default;

            ModelParams(const ModelParams &) = default;
            ModelParams &operator=(const ModelParams &) = default;

            ModelParams(ModelParams &&) noexcept = default;
            ModelParams &operator=(ModelParams &&) noexcept = default;
        };
    }
}

#endif // INCLUDE_ASTRA_RP_MODEL_PARAMS_HPP