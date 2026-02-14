#pragma once

#include "global.h"


//PO3's hook used to disable all lights tot start wiht a clean base
struct TESObjectLIGH_GenDynamic {
    static RE::NiPointLight* thunk(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref, RE::NiNode* node,
        bool forceDynamic, bool useLightRadius, bool affectRequesterOnly);

    static inline REL::Relocation<decltype(thunk)> func;

    static bool shouldDisableLight(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref);
    static void Install();
};

// not very usefull problem is not # of lights in a cell but on a bs tri shape
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

// I use this to calculate the overlap of a light with the camera and disable lights outside the camera
// it works but did not seem to help. I was told that the engine already does this but when you press the menu button 
// to load light templates it displays a debug log of all active lights and without this lights still seem to be active so idk if enigne does this or not already
inline void NiCamera_unk_CalculateFrustumOverlap(RE::NiCamera* camera, float* coord, float* result1, float* result2, float epsilon)
{
    // 140C65760
    using func_t = decltype(&NiCamera_unk_CalculateFrustumOverlap);
    static REL::Relocation<func_t> func{ REL::VariantID(69265, 70632, 0) };
    func(camera, coord, result1, result2, epsilon);
}

// I was told vvanilla does this I need to find it if it does already, a log dump is done every time you enabe light editor 
// it logs all active lights and there data. according to this all lights are still active meaning vanilla does not but it needs to be investigated further
template <class T>
inline void disableLightsNotInCamera(T& lights, RE::ShadowSceneNode* ssNode, RE::NiCamera* camera) {

    static std::vector<RE::NiPointer<RE::BSLight>> removedLights;
    

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
        NiCamera_unk_CalculateFrustumOverlap(camera, coord, minOut, maxOut, 0.0001f);
        bool visible = true;
        for (int i = 0; i < 3; i++) {
            if (maxOut[i] <= minOut[i]) {
                visible = false;
                break;
            }
        }
        if (visible) {
            logger::info("  Light restored! {}", nilight->name.c_str());
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
            removedLights.push_back(light);
            ssNode->RemoveLight(light);
        }
    }
} 