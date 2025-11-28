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
//backup light data for a unsused tesobjectligh we change values of.
extern LightConfig g_backup = {};

//extern std::map<LightConfig, std::vector<RE::NiPointer<RE::NiAVObject>>> niPointLightNodeBank;

 void Initialize() {
    logger::info("loading forms");
    auto dataHandler = RE::TESDataHandler::GetSingleton(); // single instance

    // below is a unused light base object . I need it to pass as arguments for ni point light generator function
 // the ni point light will inherit the data from this object like color size brighness ect. 
    LoadScreenLightMain = dataHandler->LookupForm<RE::TESObjectLIGH>(0x00105300, "Skyrim.esm");
    if (!LoadScreenLightMain) {
        logger::info("TESObjectLIGH LoadScreenLightMain (0x00105300) not found");
    }
}
// checs if fake lights should be disabled by checking some user settings. and excluding dynamicform lights
// or whitelisted lights by checking the plugin name or carryable or shadowcasters lol
 bool should_disable_light(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref,const std::string& modName)
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
 bool excludeLightEditorID(const RE::TESObjectLIGH* light) {

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

// TODO:: implement a function that can read light flags and turn into vector of strings. 
template <class T>
REX::EnumSet<RE::TES_LIGHT_FLAGS, std::uint32_t>
ParseLightFlags(const T& obj)
{
    REX::EnumSet<RE::TES_LIGHT_FLAGS, std::uint32_t> flags;

    for (const auto& flagStr : obj.flags) {
        auto it = kLightFlagMap.find(flagStr);
        if (it != kLightFlagMap.end()) {
            flags.set(it->second);
        }
    }
    return flags;
}

/* void BackupLightData()
{
    if (!LoadScreenLightMain) return;

    g_backup.fade = LoadScreenLightMain->fade;
    g_backup.radius = LoadScreenLightMain->data.radius;
    g_backup.RGBvalues = {
        static_cast<int>(LoadScreenLightMain->data.color.red),
        static_cast<int>(LoadScreenLightMain->data.color.green),
        static_cast<int>(LoadScreenLightMain->data.color.blue)
    };
    //TODO:: Implement function to parse RE::TESObjectLIGH flags to string vector
    g_backup.flags = LoadScreenLightMain->data.flags
} */

/*void RestoreLightData()
{
    if (!LoadScreenLightMain) return;

    LoadScreenLightMain->fade = g_backup.fade;
    LoadScreenLightMain->data.radius = g_backup.radius;
    LoadScreenLightMain->data.color.red = g_backup.RGBValues[0];
    LoadScreenLightMain->data.color.green = g_backup.RGBValues[1];
    LoadScreenLightMain->data.color.blue = g_backup.RGBValues[2];

    LoadScreenLightMain->data.flags = ParseLightFlags(g_backup.flags);
 }*/

 void SetTESObjectLIGHData(const LightConfig& config) {
    LoadScreenLightMain->fade = config.fade;
    LoadScreenLightMain->data.radius = config.radius;
    LoadScreenLightMain->data.color.red = config.RGBValues[0];
    LoadScreenLightMain->data.color.green = config.RGBValues[1];
    LoadScreenLightMain->data.color.blue = config.RGBValues[2];
    LoadScreenLightMain->data.flags = ParseLightFlags(config);
}

 void ApplyLightPosition(RE::NiPointLight* light, const LightConfig& cfg)
{
    if (!light) return;  // safety check

    light->local.translate.x = cfg.position[0];
    light->local.translate.y = cfg.position[1];
    light->local.translate.z = cfg.position[2];
}

/*// on startup store a bunch of niobects that are nipoint lights wrapped in ni pointers (safety) in a bank before main menu loads
// this way no lights are genereated during gameplay
 void CreateNiPointLightsFromJSONAndFillBank() {
    logger::info("Assigning niPointLight... total groups: {}", niPointLightNodeBank.size());

    // BackupLightData();

    for (auto& pair : niPointLightNodeBank) {
        const auto& jsonCfg = pair.first;
        auto& bankedNodes = pair.second;

        // Apply current config data to the template light so nipoint light will inherit it
        SetTESObjectLIGHData(jsonCfg);

        // Create NiPointLight 
        RE::NiPointLight* niPointLight = Hooks::TESObjectLIGH_GenDynamic::func(
            LoadScreenLightMain,   // Template TESObjectLIGH
            nullptr,               // Reference to attach to (we attach later and to meshes not refs)
            nullptr,               // Node to attach to ( this is more specifically where on the ref mabye?)
            false,                 // forceDynamic
            true,                  // useLightRadius
            false                  // affectRequesterOnly
        );

        // Apply position from the JSON config
        ApplyLightPosition(niPointLight, jsonCfg);

        // how many nodes to create based on type, tends to be alot of candles 25 is overkill for others. 
        const size_t maxNodes = (jsonCfg.nodeName == "candle") ? 60 : 20;

        for (size_t i = 0; i < maxNodes; ++i) {
            // Clone the NiPointLight as a NiObject
            auto clonedNiPointLightAsNiObject = CloneNiPointLight(niPointLight);

            if (!clonedNiPointLightAsNiObject) {
                logger::error("Failed to clone NiPointLight for node '{}'", jsonCfg.nodeName);
                continue;
            }

            RE::NiPointer<RE::NiAVObject> clonedNiPointLightAsNiObjectPtr = clonedNiPointLightAsNiObject;

            // Add the cloned light to the bank, we will attach it to meshes later
            bankedNodes.push_back(clonedNiPointLightAsNiObjectPtr);
        }
    }
    logger::info("Finished assignClonedNodes");
}*/

 void CreateNiPointLightsFromJSONAndFillBank() {
     logger::info("Assigning niPointLight... total groups: {}", niPointLightNodeBank.size());

     if (!LoadScreenLightMain) {
         logger::error("LoadScreenLightMain is null! Cannot generate lights.");
         return;
     }

     for (auto& pair : niPointLightNodeBank) {
         const auto& jsonCfg = pair.first;
         auto& bankedNodes = pair.second;

         logger::info("attempting to set data");
         // Apply current config data to the template light
         SetTESObjectLIGHData(jsonCfg);
         
         logger::info("finished setting light data.. creating light");

         // Create NiPointLight 
         RE::NiPointLight* niPointLight = Hooks::TESObjectLIGH_GenDynamic::func(
             LoadScreenLightMain, // Template TESObjectLIGH
             nullptr,             // Reference to attach to ? 
             nullptr,             // Node to attach to ? 
             false,               // forceDynamic
             true,                // useLightRadius
             false                // affectRequesterOnly
         );

         if (!niPointLight) {
             logger::error("TESObjectLIGH_GenDynamic returned null for node '{}'", jsonCfg.nodeName);
             continue;
         }

         // Apply position from the JSON config
         ApplyLightPosition(niPointLight, jsonCfg);
         
         logger::info("fsetting up max nodes");
         const size_t maxNodes = (jsonCfg.nodeName == "candle") ? 60 : 20;

         for (size_t i = 0; i < maxNodes; ++i) {
             auto clonedNiPointLightAsNiObject = CloneNiPointLight(niPointLight);
             if (!clonedNiPointLightAsNiObject) {
                 logger::error("Failed to clone NiPointLight for node '{}' (iteration {})", jsonCfg.nodeName, i);
                 continue;
             }
             logger::info("wrapping in pointers");
             RE::NiPointer<RE::NiAVObject> clonedNiPointLightAsNiObjectPtr = clonedNiPointLightAsNiObject;
             if (!clonedNiPointLightAsNiObjectPtr) {
                 logger::error("Cloned NiPointer is null for node '{}' (iteration {})", jsonCfg.nodeName, i);
                 continue;
             }
             logger::info("fadding to bank. ");
             bankedNodes.push_back(clonedNiPointLightAsNiObjectPtr);
             logger::debug("Added cloned light for node '{}' (iteration {})", jsonCfg.nodeName, i);
         }
     }

     logger::info("Finished assignClonedNodes");
 }



