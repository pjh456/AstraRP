#ifndef INCLUDE_ASTRA_RP_TYPES_HPP
#define INCLUDE_ASTRA_RP_TYPES_HPP

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>

namespace astra_rp
{
    using Str = std::string;

    template <typename T>
    using Vec = std::vector<T>;

    template <typename K, typename V>
    using Map = std::map<K, V>;

    template <typename K, typename V>
    using HashMap = std::unordered_map<K, V>;

    template <typename T>
    using UniPtr = std::unique_ptr<T>;

    template <typename T>
    using MulPtr = std::shared_ptr<T>;
}

#endif // INCLUDE_ASTRA_RP_TYPES_HPP