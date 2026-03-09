#ifndef INCLUDE_ASTRA_RP_ERROR_HPP
#define INCLUDE_ASTRA_RP_ERROR_HPP

#include <source_location>
#include <sstream>
#include <string>
#include <memory>

#include "utils/result.hpp"

namespace astra_rp
{
    using Str = std::string;

    template <typename T>
    using MulPtr = std::shared_ptr<T>;

    namespace utils
    {
        class ErrorBuilder;

        enum class ErrorDomain
        {
            Core,     // 模型、上下文、Batch、分词等底层结构
            Infer,    // 推理会话、引擎解码、采样
            Pipeline, // DAG图、节点执行、调度器
            System    // 内存分配、文件IO等
        };

        inline const char *domain_to_string(ErrorDomain domain) noexcept
        {
            switch (domain)
            {
            case ErrorDomain::Core:
                return "CORE";
            case ErrorDomain::Infer:
                return "INFER";
            case ErrorDomain::Pipeline:
                return "PIPELINE";
            case ErrorDomain::System:
                return "SYSTEM";
            default:
                return "UNKNOWN";
            }
        }

        enum class ErrorCode
        {
            // --- Core Errors ---
            ModelLoadFailed,
            LoRALoadFailed,
            ContextInitFailed,
            BatchCapacityExceeded,
            BatchAcquireFailed,
            BatchAddFailed,
            TokenizeFailed,
            DetokenizeFailed,

            // --- Infer Errors ---
            EngineDecodeFailed,
            SessionStateInvalid,
            SamplerConfigurationError,

            // --- Pipeline Errors ---
            GraphCycleDetected,
            NodeExecutionFailed,
            MissingDependency,

            // --- System/General Errors ---
            InvalidArgument,
            OutOfMemory,

            UnknownError
        };

        class Error
        {
            friend class ErrorBuilder;

        private:
            ErrorDomain m_domain = ErrorDomain::System;
            ErrorCode m_code = ErrorCode::UnknownError;
            Str m_msg;
            Str m_context_id;
            std::source_location m_loc;
            MulPtr<Error> m_cause = nullptr;

        public:
            Error() : m_loc(std::source_location::current()) {}

        public:
            ErrorDomain domain() const noexcept { return m_domain; }
            ErrorCode code() const noexcept { return m_code; }
            const Str &message() const noexcept { return m_msg; }
            const Str &context_id() const noexcept { return m_context_id; }
            MulPtr<Error> cause() const noexcept { return m_cause; }

        public:
            Str to_string() const
            {
                std::ostringstream oss;
                oss << "[" << domain_to_string(m_domain) << "] ";

                if (!m_context_id.empty())
                {
                    oss << "<" << m_context_id << "> ";
                }

                oss << m_msg
                    << " (at " << m_loc.file_name() << ":" << m_loc.line()
                    << " in " << m_loc.function_name() << ")";

                if (m_cause)
                    oss << "\n  Caused by: " << m_cause->to_string();

                return oss.str();
            }
        };

        class ErrorBuilder
        {
        private:
            Error m_err;

        public:
            ErrorBuilder(std::source_location loc = std::source_location::current())
            {
                m_err.m_loc = loc;
            }

            ErrorBuilder(const Error &base_err) : m_err(base_err) {}

            Error build() const noexcept { return m_err; }

        public:
            ErrorBuilder &domain(ErrorDomain dom) noexcept
            {
                m_err.m_domain = dom;
                return *this;
            }

            ErrorBuilder &core() noexcept { return domain(ErrorDomain::Core); }
            ErrorBuilder &infer() noexcept { return domain(ErrorDomain::Infer); }
            ErrorBuilder &pipeline() noexcept { return domain(ErrorDomain::Pipeline); }
            ErrorBuilder &system() noexcept { return domain(ErrorDomain::System); }

        public:
            ErrorBuilder &code(ErrorCode c) noexcept
            {
                m_err.m_code = c;
                return *this;
            }

            // --- Core Errors ---
            ErrorBuilder &model_load_failed() noexcept { return code(ErrorCode::ModelLoadFailed); }
            ErrorBuilder &lora_load_failed() noexcept { return code(ErrorCode::LoRALoadFailed); }
            ErrorBuilder &context_init_failed() noexcept { return code(ErrorCode::ContextInitFailed); }
            ErrorBuilder &batch_capacity_exceeded() noexcept { return code(ErrorCode::BatchCapacityExceeded); }
            ErrorBuilder &batch_acquire_failed() noexcept { return code(ErrorCode::BatchAcquireFailed); }
            ErrorBuilder &batch_add_failed() noexcept { return code(ErrorCode::BatchAddFailed); }
            ErrorBuilder &tokenize_failed() noexcept { return code(ErrorCode::TokenizeFailed); }
            ErrorBuilder &detokenize_failed() noexcept { return code(ErrorCode::DetokenizeFailed); }

            // --- Infer Errors ---
            ErrorBuilder &engine_decode_failed() noexcept { return code(ErrorCode::EngineDecodeFailed); }
            ErrorBuilder &session_state_invalid() noexcept { return code(ErrorCode::SessionStateInvalid); }
            ErrorBuilder &sampler_configuration_error() noexcept { return code(ErrorCode::SamplerConfigurationError); }

            // --- Pipeline Errors ---
            ErrorBuilder &graph_cycle_detected() noexcept { return code(ErrorCode::GraphCycleDetected); }
            ErrorBuilder &node_execution_failed() noexcept { return code(ErrorCode::NodeExecutionFailed); }
            ErrorBuilder &missing_dependency() noexcept { return code(ErrorCode::MissingDependency); }

            // --- System/General Errors ---
            ErrorBuilder &invalid_argument() noexcept { return code(ErrorCode::InvalidArgument); }
            ErrorBuilder &out_of_memory() noexcept { return code(ErrorCode::OutOfMemory); }

            ErrorBuilder &unknown_error() noexcept { return code(ErrorCode::UnknownError); }

        public:
            ErrorBuilder &message(Str msg) noexcept
            {
                m_err.m_msg = std::move(msg);
                return *this;
            }

            ErrorBuilder &context_id(Str ctx_id) noexcept
            {
                m_err.m_context_id = std::move(ctx_id);
                return *this;
            }

            ErrorBuilder &wrap(const Error &cause) noexcept
            {
                m_err.m_cause =
                    std::make_shared<Error>(cause);
                return *this;
            }

            ErrorBuilder &wrap(Error &&cause) noexcept
            {
                m_err.m_cause =
                    std::make_shared<Error>(std::move(cause));
                return *this;
            }
        };
    }
}

#endif // INCLUDE_ASTRA_RP_ERROR_HPP