#pragma once

#include "logger.hpp"
#include "global.h"


struct BSLight_AddFadeNode
{
    static __int64 thunk(RE::BSLight* a_this, RE::NiAVObject* a_root);
    static inline std::uintptr_t func;
    static void Install();
};

struct BSShaderPropertyLightData_AttachLight
{
    static __int64 thunk(RE::BSShaderPropertyLightData* a_this, RE::BSLight* light);
    static inline std::uintptr_t func;
    static void Install();
};
