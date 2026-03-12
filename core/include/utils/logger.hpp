#ifndef INCLUDE_ASTRA_RP_LOGGER_HPP
#define INCLUDE_ASTRA_RP_LOGGER_HPP

#include <iostream>
#include <chrono>
#include <mutex>
#include <iomanip>
#include <functional>

#include "core/init_manager.hpp"
#include "utils/types.hpp"

namespace astra_rp
{
    namespace utils
    {
        enum class LogLevel
        {
            DEBUG,
            INFO,
            WARN,
            ERROR,
            FATAL
        };

        inline const char *log_level2str(LogLevel level)
        {
            switch (level)
            {
            case LogLevel::DEBUG:
                return "DEBUG";
            case LogLevel::INFO:
                return "INFO";
            case LogLevel::WARN:
                return "WARN";
            case LogLevel::ERROR:
                return "ERROR";
            case LogLevel::FATAL:
                return "FATAL";
            default:
                return "UNKNOWN";
            }
        }

        class Logger
        {
        public:
            using LogCallback =
                std::function<void(
                    LogLevel,
                    const Str &,
                    int,
                    const Str &)>;

        private:
            std::mutex m_mtx;
            LogCallback m_callback = nullptr;

            astra_rp::core::InitManager &m_init;

        public:
            static Logger &instance()
            {
                static Logger manager;
                return manager;
            }

        private:
            Logger()
                : m_init(astra_rp::core::
                             InitManager::instance()) {}

        public:
            ~Logger() = default;

            Logger(const Logger &) = delete;
            Logger &operator=(const Logger &) = delete;

            Logger(Logger &&) noexcept = default;
            Logger &operator=(Logger &&) noexcept = default;

        public:
            void set_callback(LogCallback cb)
            {
                std::lock_guard<std::mutex> lock(m_mtx);
                m_callback = std::move(cb);
            }

            void log(LogLevel level, const Str &file, int line, const Str &msg)
            {
                std::lock_guard<std::mutex> lock(m_mtx);
                if (m_callback)
                    m_callback(level, file, line, msg);

                auto now = std::chrono::system_clock::now();
                auto time = std::chrono::system_clock::to_time_t(now);

                std::cerr << "[" << std::put_time(std::localtime(&time), "%H:%M:%S") << "] "
                          << "[" << log_level2str(level) << "] "
                          << "[" << file << ":" << line << "] "
                          << msg << std::endl;

                if (level == LogLevel::FATAL)
                    std::exit(1);
            }

        public:
            static void print(LogLevel level, const char *file, int line, const Str &msg)
            {
                instance().log(level, file, line, msg);
            }

            static void debug(const char *file, int line, const Str &msg)
            {
                print(LogLevel::DEBUG, file, line, msg);
            }

            static void info(const char *file, int line, const Str &msg)
            {
                print(LogLevel::INFO, file, line, msg);
            }

            static void warn(const char *file, int line, const Str &msg)
            {
                print(LogLevel::WARN, file, line, msg);
            }

            static void error(const char *file, int line, const Str &msg)
            {
                print(LogLevel::ERROR, file, line, msg);
            }

            static void fatal(const char *file, int line, const Str &msg)
            {
                print(LogLevel::FATAL, file, line, msg);
            }
        };
    }
}

#ifdef ASTRA_IGNORE_LOG
#define ASTRA_LOG(level, msg)
#elif defined ASTRA_SIMPLE_LOG
#define ASTRA_LOG(level, msg)                                                  \
    do                                                                         \
    {                                                                          \
        std::ostringstream oss;                                                \
        oss << msg;                                                            \
        astra_rp::utils::Logger::print(level, "Ignored", __LINE__, oss.str()); \
    } while (0)
#else
#define ASTRA_LOG(level, msg)                                                 \
    do                                                                        \
    {                                                                         \
        std::ostringstream oss;                                               \
        oss << msg;                                                           \
        astra_rp::utils::Logger::print(level, __FILE__, __LINE__, oss.str()); \
    } while (0)
#endif // ASTRA_LOG

#define ASTRA_LOG_DEBUG(msg) ASTRA_LOG(astra_rp::utils::LogLevel::DEBUG, msg)
#define ASTRA_LOG_INFO(msg) ASTRA_LOG(astra_rp::utils::LogLevel::INFO, msg)
#define ASTRA_LOG_WARN(msg) ASTRA_LOG(astra_rp::utils::LogLevel::WARN, msg)
#define ASTRA_LOG_ERROR(msg) ASTRA_LOG(astra_rp::utils::LogLevel::ERROR, msg)

#endif // INCLUDE_ASTRA_RP_LOGGER_HPP