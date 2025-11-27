#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <RE/T/TESObjectLIGH.h>
#include "config.hpp"

#define FOREACH_LIGHTFLAG(F) \
F(kNone) \
F(kDynamic) \
F(kCanCarry) \
F(kNegative) \
F(kFlicker) \
F(kDeepCopy) \
F(kOffByDefault) \
F(kFlickerSlow) \
F(kPulse) \
F(kPulseSlow) \
F(kSpotlight) \
F(kSpotShadow) \
F(kHemiShadow) \
F(kOmniShadow) \
F(kPortalStrict) \

// I use this unused TESObject::Ligh
struct LoadScreenLightMainBackupData {
    float fade;
    std::uint32_t radius;
    std::array<int, 3> RGBvalues;
    std::vector<std::string> flags;
};

// Try to exclude light by editorID.
inline bool excludeLightEditorID(const RE::TESObjectLIGH* light) {

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

inline LoadScreenLightMainBackupData g_backup;

inline std::vector<std::string> LightFlagsToStrings(const REX::EnumSet<RE::TES_LIGHT_FLAGS, std::uint32_t>& flags) {
    
}

inline REX::EnumSet<RE::TES_LIGHT_FLAGS, std::uint32_t> ParseLightFlagsFromString(const std::vector<std::string>& flags) {

}

inline void BackupLightData()
{
    if (!LoadScreenLightMain) return;

    g_backup.fade = LoadScreenLightMain->fade;
    g_backup.radius = LoadScreenLightMain->data.radius;
    g_backup.RGBvalues = {
        static_cast<int>(LoadScreenLightMain->data.color.red),
        static_cast<int>(LoadScreenLightMain->data.color.green),
        static_cast<int>(LoadScreenLightMain->data.color.blue)
    };
    //to do implemetnlight flags to strings. 
    g_backup.flags = LightFlagsToStrings(LoadScreenLightMain->data.flags);
}

inline void RestoreLightData()
{
    if (!LoadScreenLightMain) return;

    LoadScreenLightMain->fade = g_backup.fade;
    LoadScreenLightMain->data.radius = g_backup.radius;
    LoadScreenLightMain->data.color.red   = g_backup.RGBvalues[0];
    LoadScreenLightMain->data.color.green = g_backup.RGBvalues[1];
    LoadScreenLightMain->data.color.blue  = g_backup.RGBvalues[2];
    LoadScreenLightMain->data.flags = ParseLightFlagsFromString(g_backup.flags);
}

inline void SetTESObjectLIGHData(const LightConfig& config){
LoadScreenLightMain->fade = config.fade;
    LoadScreenLightMain->data.radius = config.radius;
    LoadScreenLightMain->data.color.red   = config.RGBValues[0];
    LoadScreenLightMain->data.color.green = config.RGBValues[1];
    LoadScreenLightMain->data.color.blue  = config.RGBValues[2];
LoadScreenLightMain->data.flags = ParseLightFlagsFromString(config.flags);
}

inline void ApplyLightPosition(RE::NiPointLight* light, const LightConfig& cfg)
{
    if (!light) return;  // safety check

    light->local.translate.x = cfg.position[0];
    light->local.translate.y = cfg.position[1];
    light->local.translate.z = cfg.position[2];
}

