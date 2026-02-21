#pragma once

#include "global.h"
#include <unordered_set>


struct NiPointerHasher
{
    std::size_t operator()(const RE::NiPointer<RE::BSLight>& ptr) const noexcept
    {
        return std::hash<RE::BSLight*>{}(ptr.get());
    }
};

//PO3's hook used to disable all lights tot start wiht a clean base
struct TESObjectLIGH_GenDynamic {
    static RE::NiPointLight* thunk(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref, RE::NiNode* node,
        bool forceDynamic, bool useLightRadius, bool affectRequesterOnly);

    static inline REL::Relocation<decltype(thunk)> func;

    static bool shouldDisableLight(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref);
    static void Install();
};

// not very usefull problem is not # of lights in a cell but # of lights on a bs tri shape
template <class T>
inline void disableLightsPastMaxDistance(T& lights, RE::NiPoint3& playerPos, RE::ShadowSceneNode* ssNode) {
    for (auto& light : lights) {
        if (!light) continue;
        RE::NiPoint3 lightPos = light->worldTranslate;

        if (lightPos.GetDistance(playerPos) >= globals::maxLightDistance) {
            ssNode->RemoveLight(light);
        }
    }
}

// Took in from mehs intellighent mod
inline void NiCamera_unk_CalculateFrustumOverlap(RE::NiCamera* camera, float* coord, float* result1, float* result2, float epsilon)
{
    // 140C65760
    using func_t = decltype(&NiCamera_unk_CalculateFrustumOverlap);
    static REL::Relocation<func_t> func{ REL::VariantID(69265, 70632, 0) };
    func(camera, coord, result1, result2, epsilon);
}

// rurns our engine already does this, (frustrum culling member on ni light) only thing better would be
//to disable lights behind walls out of player sight but idk how expensive ray casting is. (mabye viable for shadow casters?)
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
} 