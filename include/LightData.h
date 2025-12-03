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

// extend ni point light runtime data so ISL sees our lights otherwise darkness
struct ISL_Overlay
{
    std::uint32_t flags;       // I dont think we need this but idk can ignore for now
    float         cutoffOverride;//ISL need for isl from config
    RE::FormID    lighFormId;
    RE::NiColor   diffuse;
    float         radius;
    float         pad1C;
    float         size; //ISL need for isl from config
    float         fade;
    std::uint32_t unk138;

    static ISL_Overlay* Get(RE::NiLight* niLight)
    {
        return reinterpret_cast<ISL_Overlay*>(&niLight->GetLightRuntimeData());
    }
};

// Backup light data for unused TESObjectLIGH

struct LightData {

    static bool isISL;
    // Light flags
    /*#define FOREACH_LIGHTFLAG(F) \
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
    };*/
    static void assignNiPointLightsToBank();

    static bool shouldDisableLight(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref, const std::string& modName);
    static bool excludeLightEditorID(const RE::TESObjectLIGH* light);
   // template <class T>
   // inline REX::EnumSet<RE::TES_LIGHT_FLAGS, std::uint32_t> ParseLightFlags(const T& obj);
   static void setNiPointLightAmbientAndDiffuse(RE::NiPointLight* niPointLight, const LightConfig& cfg);
   static void setNiPointLightData(RE::NiPointLight* niPointLight, const LightConfig& cfg,RE::TESObjectLIGH* lighTemplate);
   static void setNiPointLightPos(RE::NiPointLight* light, const LightConfig& cfg);
   static RE::NiPoint3 getNiPointLightRadius(const LightConfig& cfg);
   static RE::NiPointLight* createNiPointLight();
   static void setISLData(RE::NiPointLight* niPointLight, const LightConfig& cfg);
   static void setISLFlag(RE::TESObjectLIGH* ligh); 
   static RE::ShadowSceneNode::LIGHT_CREATE_PARAMS makeLightParams(const LightConfig& cfg);
   static void attachNiPointLightToShadowSceneNode(RE::NiPointLight* niPointLight, const LightConfig& cfg);
   static void printLightParams(const RE::ShadowSceneNode::LIGHT_CREATE_PARAMS& params) {
       logger::debug(" shadowLight	 {}", params.shadowLight);
       logger::debug(" portalStrict  {}", params.portalStrict);
       logger::debug(" affectLand	 {}", params.affectLand);
       logger::debug(" affectWater	 {}", params.affectWater);
       logger::debug(" neverFades	 {}", params.neverFades);
       logger::debug(" fov			 {}", params.fov);
       logger::debug(" falloff		 {}", params.falloff);
       logger::debug(" nearDistance  {}", params.nearDistance);
       logger::debug(" depthBias	 {}", params.depthBias);
   }
   // void initialize();
};
