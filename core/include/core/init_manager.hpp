#ifndef INCLUDE_ASTRA_RP_INIT_MANAGER_HPP
#define INCLUDE_ASTRA_RP_INIT_MANAGER_HPP

namespace astra_rp
{
    namespace core
    {
        class InitManager
        {
        public:
            static InitManager &instance();

        private:
            InitManager();
            ~InitManager();

        public:
            InitManager(const InitManager &) = delete;
            InitManager &operator=(const InitManager &) = delete;

            InitManager(InitManager &&) noexcept = default;
            InitManager &operator=(InitManager &&) noexcept = default;
        };
    }
}

#endif // INCLUDE_ASTRA_RP_INIT_MANAGER_HPP