#include "core/model.hpp"

#include "common.h"

#include <iostream>
#include <stdexcept>

namespace astra_rp
{
    namespace core
    {
        // 用于保证 backend 只初始化一次
        static std::once_flag g_backend_init_flag;

        void Model::init_backend()
        {
            std::call_once(
                g_backend_init_flag,
                []()
                {
                    // 初始化 llama.cpp 的后端 (CPU/CUDA/Metal 等)
                    // numa: false (是否使用非统一内存访问优化，通常设为 false)
                    llama_backend_init();

                    // TODO: 在程序退出时注册清理函数，或者在单例析构中调用
                    // std::atexit([]() { llama_backend_free(); });
                    std::cerr << "[AstraRP] Backend initialized." << std::endl;
                });
        }

        Model::Model(const Str &path) : m_path(path)
        {
            // 1. 确保环境已初始化
            init_backend();

            // 2. 配置模型加载参数
            auto model_params = llama_model_default_params();
            model_params.n_gpu_layers = 0;

            // 3. 加载模型
            // c_str() 是因为底层 C 接口需要 const char*
            m_model = llama_load_model_from_file(m_path.c_str(), model_params);

            if (!m_model)
            {
                throw std::runtime_error("Failed to load model from: " + m_path);
            }

            std::cout << "[AstraRP] Model loaded successfully: " << m_path << std::endl;
        }

        Model::~Model()
        {
            if (m_model)
            {
                llama_free_model(m_model);
                m_model = nullptr;
                std::cout << "[AGP] Model freed." << std::endl;
            }
        }

        Vec<int> Model::tokenize(const Str &text, bool add_bos, bool special) const
        {
            if (!m_model)
                return {};

            // 1. 获取词表上限 (通常稍大于 text.size())
            // 添加 buffer 空间以防万一
            int n_tokens = text.length() + add_bos;
            std::vector<int> tokens(n_tokens);

            // 2. 调用底层 tokenize API
            auto vocab = llama_model_get_vocab(m_model);
            n_tokens = llama_tokenize(
                vocab,
                text.c_str(),
                text.length(),
                reinterpret_cast<llama_token *>(tokens.data()), // 强转 vector 数据指针
                tokens.size(),
                add_bos, // 是否添加开始符 (通常 Prompt 开头需要)
                special  // 是否解析特殊字符
            );

            if (n_tokens < 0)
            {
                // 空间不足时，llama_tokenize 会返回负数
                // 这里简单处理：重新分配更大的空间再试一次 (略，通常 text.length() 足够)
                throw std::runtime_error("Tokenization failed: buffer too small");
            }

            // 3. 调整 vector 大小为实际 token 数
            tokens.resize(n_tokens);
            return tokens;
        }

        Str Model::detokenize(int token_id) const
        {
            if (!m_model)
                return "";

            // 这是一个简单的 buffer，用于存放单个 token 对应的字符串
            // 大多数 token 对应的字符串都很短
            char buf[256];
            auto vocab = llama_model_get_vocab(m_model);
            int n = llama_token_to_piece(vocab, (llama_token)token_id, buf, sizeof(buf), 0, true);

            if (n < 0)
            {
                // 处理异常情况
                return "";
            }

            return std::string(buf, n);
        }
    }
}