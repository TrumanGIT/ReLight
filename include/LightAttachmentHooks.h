#pragma once

#include "LightManager.h"
#include "Utility.h"
#include "global.h"

//when we attach ni poiny lights to static objects
struct Load3D {

    static RE::NiAVObject* thunk(RE::TESObjectREFR* a_this, bool a_backgroundLoading);

    static inline REL::Relocation<decltype(thunk)> func;

    static constexpr std::size_t idx{ 0x6A };

    static void Install();
};


// used for scripted fires like castle volkihar that only turn on when activated
struct Activate {

    static bool thunk(
        RE::TESObjectACTI* a_this,
        RE::TESObjectREFR* a_targetRef,
        RE::TESObjectREFR* a_activatorRef,
        std::uint8_t a_arg3,
        RE::TESBoundObject* a_object,
        std::int32_t a_targetCount);

    static inline REL::Relocation<decltype(thunk)> func;

    static constexpr std::size_t idx{ 0x37 };

    static void Install();
};
