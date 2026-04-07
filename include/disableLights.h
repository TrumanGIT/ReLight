#pragma once

#include "global.h"
#include <unordered_set>
#include "LightData.h"
//#include <chrono>

//PO3's hook used to disable all lights tot start wiht a clean base
struct TESObjectLIGH_GenDynamic {
    static RE::NiPointLight* thunk(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref, RE::NiNode* node,
        bool forceDynamic, bool useLightRadius, bool affectRequesterOnly);

    static inline REL::Relocation<decltype(thunk)> func;

    static bool shouldDisableLight(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref);
    static void Install();
};

// meh321s hook from intellightent
struct BSLightingShaderProperty_IsLightAffectingSurface
{
    static bool thunk(RE::BSLightingShaderProperty* p, RE::BSLight* light);
    static inline REL::Relocation<decltype(thunk)> func;
    static void Install();
};


inline void removeActiveShadowLightsForConfig(uint32_t configID)
{
    auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
    if (!ssNode) {
        logger::warn("ShadowSceneNode[0] is null cant reinitialize lights");
        return;
    }

    std::vector<RE::NiPointer<RE::BSLight>> lightsToRemove;

    // shadow lights are persistant so must remove them first.
    for (const auto& l : ssNode->activeShadowLights) {
        if (!l || !l->light)
            continue;

        if (l->light->unk138 != configID) continue;
        lightsToRemove.push_back(l);
    }

    // remove using underlying light
    for (const auto& light : lightsToRemove) {
        ssNode->RemoveLight(light);
    }

}