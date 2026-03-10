#ifndef INCLUDE_ASTRA_RP_BINDING_CPP
#define INCLUDE_ASTRA_RP_BINDING_CPP

#include <napi.h>

#include "core/global_config.hpp"

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    return exports;
}

NODE_API_MODULE(astrarp_node, Init)

#endif // INCLUDE_ASTRA_RP_BINDING_CPP