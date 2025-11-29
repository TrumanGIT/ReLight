#pragma once
#include <cstdint>
#include "global.h"
#include "logger.hpp"
#include "ClibUtil/EditorID.hpp"
#include "Functions.h"
#include "config.hpp"
#include "LightData.h"
#include <string>
#include <vector>


    float ambientRatio = 0.1f;

     bool LightData::shouldDisableLight(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref, const std::string& modName)
    {
        if (!ref || !light || ref->IsDynamicForm()) {
            return false;
        }

        auto player = RE::PlayerCharacter::GetSingleton();

        if (IsInSoulCairnOrApocrypha(player)) {
            logger::info("player is in apocrypha or soul cairn so we should not disable light");
            return false;
        }
        if (disableShadowCasters == false &&
            light->data.flags.any(RE::TES_LIGHT_FLAGS::kOmniShadow,
                RE::TES_LIGHT_FLAGS::kHemiShadow, RE::TES_LIGHT_FLAGS::kSpotShadow))
        {
            return false;
        }

        if (disableTorchLights == false &&
            light->data.flags.any(RE::TES_LIGHT_FLAGS::kCanCarry))
        {
            return false;
        }

        for (const auto& whitelistedMod : whitelist) {
            if (modName.find(whitelistedMod) != std::string::npos) {
                return false;
            }
        }
        return true;
    }

    // Try to exclude light by editorID.
     bool LightData::excludeLightEditorID(const RE::TESObjectLIGH* light) {

        std::string edid = clib_util::editorID::get_editorID(light);

        if (!edid.empty()) {
            for (const auto& group : keywordLightGroups) {
                if (containsAll(edid, group)) {
                    logger::info("Excluding light by editorID: {}", edid);
                    return true;
                }
            }
        }
        return false;
    }

    /* // were not using tesobject ligh flags anymore need to reimplement this.
    template <class T>
    REX::EnumSet<RE::TES_LIGHT_FLAGS, std::uint32_t>
    parseLightFlags(const T& obj)
    {
        REX::EnumSet<RE::TES_LIGHT_FLAGS, std::uint32_t> flags;

        for (const auto& flagStr : obj.flags) {
            auto it = kLightFlagMap.find(flagStr);
            if (it != kLightFlagMap.end()) {
                flags.set(it->second);
            }
        }
        return flags;
    }*/

    RE::NiPointLight* LightData::createNiPointLight() {
        auto* niPointLight = RE::NiPointLight::Create();
        if (niPointLight) {
        }
        return niPointLight; // returns nullptr if create failed
    }

     RE::NiPoint3 LightData::getNiPointLightRadius(const LightConfig& cfg)
    {
        return RE::NiPoint3(cfg.radius, cfg.radius, cfg.radius);
    }

     void LightData::setNiPointLightAmbientAndDiffuse(RE::NiPointLight* niPointLight, const LightConfig& cfg, float ambientRatio) {
        if (!niPointLight) return;
        auto& data = niPointLight->GetLightRuntimeData();

      // supposedly main color of light 
        data.diffuse.red = cfg.RGBValues[0] / 255.0f;
        data.diffuse.green = cfg.RGBValues[1] / 255.0f;
        data.diffuse.blue = cfg.RGBValues[2] / 255.0f;

		// idk about ambient after a quick google search it seems ambient is usually a fraction of diffuse
        data.ambient.red = data.diffuse.red * ambientRatio;
        data.ambient.green = data.diffuse.green * ambientRatio;
        data.ambient.blue = data.diffuse.blue * ambientRatio;
    }

     void LightData::setNiPointLightPos(RE::NiPointLight* niPointLight, const LightConfig& cfg)
    {
        if (!niPointLight) return;  // safety check
        niPointLight->local.translate.x = cfg.position[0];
        niPointLight->local.translate.y = cfg.position[1];
        niPointLight->local.translate.z = cfg.position[2];
    }

     void LightData::setNiPointLightData(RE::NiPointLight* niPointLight, const LightConfig& cfg) {
        auto& data = niPointLight->GetLightRuntimeData();
        data.fade = cfg.fade;
        data.radius = getNiPointLightRadius(cfg);
        setNiPointLightPos(niPointLight, cfg); 
		// light flags are not used in niPointLight 
            //  niPointLight->data.flags = ParseLightFlags(config);
    }

    void LightData::assignNiPointLightsToBank() {
        logger::info("Assigning niPointLight... total groups: {}", niPointLightNodeBank.size());

        for (auto& pair : niPointLightNodeBank) {
            const auto& cfg = pair.first;
            auto& bankedNodes = pair.second;

            // Create NiPointLight 
            auto niPointLight = createNiPointLight();

            if (!niPointLight) {
                logger::error("failed to create ni point light for '{} Json config'", cfg.nodeName);
                continue;
            }

            setNiPointLightData(niPointLight, cfg);

            const size_t maxNodes = (cfg.nodeName == "candle") ? 60 : 20;

            for (size_t i = 0; i < maxNodes; ++i) {
                auto clonedNiPointLightAsNiObject = CloneNiPointLight(niPointLight);
                if (!clonedNiPointLightAsNiObject) {
                    logger::error("Failed to clone NiPointLight for node '{}' (iteration {})", cfg.nodeName, i);
                    continue;
                }
                //logger::info("wrapping in pointers");
                RE::NiPointer<RE::NiAVObject> clonedNiPointLightAsNiObjectPtr = clonedNiPointLightAsNiObject;
                if (!clonedNiPointLightAsNiObjectPtr) {
                    logger::error("Cloned NiPointer is null for node '{}' (iteration {})", cfg.nodeName, i);
                    continue;
                }
              // logger::info("adding to bank. ");
                bankedNodes.push_back(clonedNiPointLightAsNiObjectPtr);
                logger::debug("Added cloned light for node '{}' (iteration {})", cfg.nodeName, i);
            }
        }
        logger::info("Finished assignClonedNodes");
    }

