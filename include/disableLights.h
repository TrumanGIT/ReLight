#pragma once

#include "global.h"
#include <unordered_set>
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

// used to take a guesstamite of bsshaderproperty light list size
/*struct PendingProperty {
    RE::NiPointer<RE::BSLightingShaderProperty> prop;
    std::chrono::steady_clock::time_point registeredAt; 
   // int lightScoreBonus = 0; 

    static std::vector<PendingProperty> list;
    static std::mutex mutex;
};*/


inline void clearSSNodeLights() {
    auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
    if (!ssNode) {
        logger::warn("ShadowSceneNode[0] is null cant reinitialize lights");
        return;
    }

    for (const auto& l : ssNode->activeLights) {
        if (!l) continue;

        std::string lightName = l->light->name.c_str();

        if (lightName.empty() || lightName[0] != 'R' || lightName[1] != 'L')
            continue;

        ssNode->RemoveLight(l);

    }

    for (const auto& l : ssNode->activeShadowLights) {
        if (!l) continue;

        std::string lightName = l->light->name.c_str();

        if (lightName.empty() || lightName[0] != 'R' || lightName[1] != 'L')
            continue;

        ssNode->RemoveLight(l);
    }
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