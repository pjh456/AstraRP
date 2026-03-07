#ifndef INCLUDE_ASTRA_RP_TYPES_HPP
#define INCLUDE_ASTRA_RP_TYPES_HPP

#include <stdint.h>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <queue>
#include <memory>

namespace astra_rp
{
    using Str = std::string;

    template <typename T>
    using Vec = std::vector<T>;

    template <typename T>
    using List = std::list<T>;

    template <typename T>
    using Queue = std::queue<T>;

    template <typename K, typename V>
    using Map = std::map<K, V>;

    template <typename K, typename V>
    using HashMap = std::unordered_map<K, V>;

    template <typename T>
    using UniPtr = std::unique_ptr<T>;

    template <typename T>
    using MulPtr = std::shared_ptr<T>;

    using Token = int32_t;
}

#endif // INCLUDE_ASTRA_RP_TYPES_HPP