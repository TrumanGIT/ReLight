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


//PO3's hook used to disable and or edit vanilla / modded esp,esm,esl plugin lights
struct TESObjectLIGH_GenDynamic {
    static RE::NiPointLight* thunk(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref, RE::NiNode* node,
        bool forceDynamic, bool useLightRadius, bool affectRequesterOnly);

    static RE::NiPointLight* magicLightThunk(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref, RE::NiNode* node,
        bool forceDynamic, bool useLightRadius, bool affectRequesterOnly);

    static inline REL::Relocation<decltype(thunk)> func;

    static inline REL::Relocation<decltype(magicLightThunk)> magicLightFunc;

    static void Install();

    static void MagicLightThunkInstall();
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
