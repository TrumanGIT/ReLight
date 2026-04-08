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

inline void menuRefreshLight(uint32_t configID)
{
    auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
    if (!ssNode) {
        logger::warn("ShadowSceneNode[0] is null cant reinitialize lights");
        return;
    }

    std::vector<RE::NiPointer<RE::BSLight>> lightsToRemove;

    std::vector<RE::NiPointer<RE::NiLight>> lightsToReAdd;

    // regular lights
    for (const auto& l : ssNode->activeLights) {
        if (!l || !l->light)
            continue;

        if (l->light->unk138 != configID) continue;

        lightsToReAdd.push_back(l->light);
        lightsToRemove.push_back(l);
    }

    // shadow lights
    for (const auto& l : ssNode->activeShadowLights) {
        if (!l || !l->light)
            continue;

        if (l->light->unk138 != configID) continue;
        lightsToReAdd.push_back(l->light);
        lightsToRemove.push_back(l);
    }

    for (const auto& l : ssNode->activeLights) {
        if (!l || !l->light)
            continue;

        if (l->light->unk138 != configID) continue;
        lightsToReAdd.push_back(l->light);
        lightsToRemove.push_back(l);
    }

    // remove using underlying light
    for (const auto& light : lightsToRemove) {
        ssNode->RemoveLight(light);
    }

    SKSE::GetTaskInterface()->AddTask([lightsToReAdd]() {
        auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
        if (!ssNode) {
            logger::warn("ShadowSceneNode[0] is null in queued light refresh task");
            return;
        }

        for (const auto& light : lightsToReAdd) {
            if (!light)
                continue;

            auto it = LightData::configIDToJsonCfg.find(light->unk138);
            if (it == LightData::configIDToJsonCfg.end()) {
                logger::warn("no config found to restore defaults from for configID {}", light->unk138);
                continue;
            }

            LightData::setNiPointLightDataFromCfg(light.get(), it->second);

            auto params = LightData::makeLightParams(it->second);
            ssNode->AddLight(light.get(), params);
        }

        // reset tri light cache
        globals::cellFullyLoaded.store(true);
        globals::secondAfterCellFullyLoaded.store(false);

        });
}