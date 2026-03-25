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
        });

}

// not very usefull problem is not # of lights in a cell but # of lights on a bs tri shape
/*template <class T>
inline void disableLightsPastMaxDistance(T& lights, RE::NiPoint3& playerPos, RE::ShadowSceneNode* ssNode) {
    for (auto& light : lights) {
        if (!light) continue;
        RE::NiPoint3 lightPos = light->worldTranslate;

        if (lightPos.GetDistance(playerPos) >= globals::maxLightDistance) {
            ssNode->RemoveLight(light);
        }
    }
 }*/

/*
template <class T>
inline void disableLightsNotInCamera(T& lights, RE::ShadowSceneNode* ssNode, RE::NiCamera* camera, RE::PlayerCharacter* player) {

    static std::unordered_set<
        RE::NiPointer<RE::BSLight>,
        NiPointerHasher
    > removedLights;

    //REenable disabled lights that were out of camera view
    for (auto it = removedLights.begin(); it != removedLights.end(); ) {
        auto light = *it;
        if (!light || !light->light.get()) {
            it = removedLights.erase(it);
            continue;
        }
        auto nilight = light->light.get();
        float radius = nilight->GetLightRuntimeData().radius.x;
        float coord[4] = {
            nilight->world.translate.x,
            nilight->world.translate.y,
            nilight->world.translate.z,
            radius
        };
        float minOut[3]{}, maxOut[3]{};
        NiCamera_unk_CalculateFrustumOverlap(camera, coord, minOut, maxOut, globals::frustumOverlapTolerance);
        bool visible = true;
        for (int i = 0; i < 3; i++) {
            if (maxOut[i] <= minOut[i]) {
                visible = false;
                break;
            }
        }
        if (visible) {

            auto playerPos = player->GetPosition(); 

            float distanceFromPlayer = playerPos.GetDistance(nilight->world.translate);

            if (distanceFromPlayer >= globals::fLODFadeOutMultObjects) return;

            logger::info("  Light {} at coord {} restored , player Pos{} distance from player {}", nilight->name.c_str(), nilight->world.translate, playerPos, distanceFromPlayer);
            ssNode->AddLight(light.get());
            it = removedLights.erase(it);
        }
        else {
            ++it;
        }
    }

    // disable lights that are out of camera view
    for (auto& light : lights) {
        if (!light || !light->light.get()) continue;
        auto nilight = light->light.get();
        float radius = nilight->GetLightRuntimeData().radius.x;
        float coord[4] = {
            nilight->world.translate.x,
            nilight->world.translate.y,
            nilight->world.translate.z,
            radius
        };
        float minOut[3]{}, maxOut[3]{};
        NiCamera_unk_CalculateFrustumOverlap(camera, coord, minOut, maxOut, 0.0001f);
        bool visible = true;
        for (int i = 0; i < 3; i++) {
            if (maxOut[i] <= minOut[i]) {
                visible = false;
                break;
            }
        }
        if (!visible) {
            logger::info("  Light removed! {}", nilight->name.c_str());
            removedLights.insert(light);
            ssNode->RemoveLight(light);
        }
    }
} */