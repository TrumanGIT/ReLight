#pragma once
#include "SKSEMenuFramework.h"
#include "logger.hpp"
#include "LightData.h"

namespace UI {

    void Register();
    void __stdcall RenderSettings();
    void __stdcall RenderLightEditor();
    void __stdcall RenderTestingMenu();
    void saveSettingsToIni();
    void getAllLights();
    void restoreLightToDefaults(RE::NiPointer<RE::NiLight> selectedLight);
    inline MENU_WINDOW reLightMenuWindow;

    inline void debugLogAllLights() {
        auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
        if (!ssNode) {
            logger::warn("ShadowSceneNode[0] is null!");
            return;
        }

        auto& rt = ssNode->GetRuntimeData();

        for (auto& light : rt.activeLights) {
            if (!light) continue;

            std::string lightName = light->light->name.c_str();

            if (lightName[0] != 'R' || lightName[1] != 'L')
                continue;

            const auto& lightRt = light->light->GetLightRuntimeData();
            auto it = LightData::configIDToJsonCfg.find(lightRt.unk138);
            if (it == LightData::configIDToJsonCfg.end()) {
                logger::warn("Config ID {:08X} not found in config map", lightRt.unk138);
                continue;  // or continue depending on your function
            }

            const auto& cfg = it->second;

            logger::debug(
                "[Light] '{}'\n"
                "  brightness        {}\n"
                "  startingBrightness{}\n"
                "  radius            {}\n"
                "  flickerIntensity  {}\n"
                "  flickersPerSecond {}\n"
                "  worldPos          {}\n"
                "  unk060            {}\n"
                "  configID          {}",
                lightName,
                lightRt.fade,
                cfg.startingFade,
                lightRt.radius,
                cfg.flickerIntensity,
                cfg.flickersPerSecond,
                light->light->world.translate,
                light->unk060,
                cfg.configID
            );

            if (globals::islInstalled) {

                auto* islRt = Overlay::Get(light->light.get());

                if (!islRt)
                    return;

                logger::debug("Light size {} light cuttoff {}", islRt->size, islRt->cutoffOverride);

            }
        }

        for (auto& light : rt.activeShadowLights) {
            if (!light) continue;

            std::string lightName = light->light->name.c_str();

            if (lightName[0] != 'R' || lightName[1] != 'L')
                continue;

            const auto& lightRt = light->light->GetLightRuntimeData();

            auto it = LightData::configIDToJsonCfg.find(lightRt.unk138);
            if (it == LightData::configIDToJsonCfg.end()) {
                logger::warn("Config ID {:08X} not found in config map", lightRt.unk138);
                continue;
            }

            const auto& cfg = it->second;

            logger::debug(
                "[Shadow Light] '{}'\n"
                "  brightness        {}\n"
                "  startingBrightness{}\n"
                "  radius            {}\n"
                "  flickerIntensity  {}\n"
                "  flickersPerSecond {}\n"
                "  worldPos          {}\n"
                "  unk060            {}\n"
                "  configID          {}",
                lightName,
                lightRt.fade,
                cfg.startingFade,
                lightRt.radius,
                cfg.flickerIntensity,
                cfg.flickersPerSecond,
                light->light->world.translate,
                light->unk060,
                cfg.configID
            );


            if (globals::islInstalled) {

                auto* islRt = Overlay::Get(light->light.get());

                if (!islRt)
                    return;

                logger::debug("Shadow Light size {} light cuttoff {}", islRt->size, islRt->cutoffOverride);

            }
        }

        logger::debug("printing all refs in attached lights set");
        for (const auto formID : globals::refsWithAttachedLights) {


            logger::debug("ref {:08X}", formID);

        }
    }
}