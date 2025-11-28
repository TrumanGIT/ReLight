#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <array>
#include <map>
#include <unordered_map>
#include "logger.hpp"
#include "ClibUtil/EditorID.hpp"
#include "config.hpp"


// Backup light data for unused TESObjectLIGH
extern LightConfig g_backup;

// Bank of NiPointLight objects wrapped in NiPointers
//extern std::map<LightConfig, std::vector<RE::NiPointer<RE::NiAVObject>>> niPointLightNodeBank;

// Light flags
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
F(kPortalStrict)

#define FLAGS2MAP(F) { #F, RE::TES_LIGHT_FLAGS::F },

inline const std::unordered_map<std::string, RE::TES_LIGHT_FLAGS> kLightFlagMap{
    FOREACH_LIGHTFLAG(FLAGS2MAP)
    {
"kType", RE::TES_LIGHT_FLAGS::kType
}
};

// Utility functions (inline for header-only)
bool should_disable_light(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref, const std::string& modName);
bool excludeLightEditorID(const RE::TESObjectLIGH* light);
template <class T>
inline REX::EnumSet<RE::TES_LIGHT_FLAGS, std::uint32_t> ParseLightFlags(const T& obj);

//inline void RestoreLightData();
 void SetTESObjectLIGHData(const LightConfig& config);
 void ApplyLightPosition(RE::NiPointLight* light, const LightConfig& cfg);
 void CreateNiPointLightsFromJSONAndFillBank();
 void Initialize();
