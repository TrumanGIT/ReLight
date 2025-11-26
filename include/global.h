#pragma once
#include <unordered_set>
#include <unordered_map>

extern bool disableShadowCasters;
extern bool disableTorchLights;
extern bool removeFakeGlowOrbs;
extern bool dataHasLoaded;

extern uint8_t red;
extern uint8_t green;
extern uint8_t blue;

extern RE::FormID soulCairnFormID;

extern RE::FormID apocryphaFormID;

extern RE::TESObjectLIGH* LoadScreenLightMain;

extern std::vector<std::string> whitelist;

extern std::vector<std::string> exclusionList;

extern std::vector<std::string> exclusionListPartialMatch;

extern std::unordered_set<RE::FormID> excludedLightFormIDs;

extern std::vector<std::string> priorityList;

//extern std::unordered_map<std::string, std::string> baseMeshesAndTemplateToAttach;

extern std::map<std::vector<LightConfig, std::vector<RE::NiPointer<RE::NiPointLight>>> NiPointLightNodeBank;

// defined as static const for potential caching
static const std::unordered_map<std::string, std::string> nordicHallMeshesAndTemplates = {
	{ "norcathallsm1way01", "Nordic Cata 1way01 Candles_NOT Animated.nif" },
	{ "norcathallsm1way02", "Nordic Cata 1way02 Candles_NOT Animated.nif" },
	{ "norcathallsm1way03", "Nordic Cata 1way03 Candles_NOT Animated.nif" },
	{ "norcathallsm2way01", "Nordic Cata 2way01 Candles_NOT Animated.nif" },
	{ "norcathallsm3way01", "Nordic Cata 3way01 Candles_NOT Animated.nif" },
	{ "norcathallsm3way02", "Nordic Cata 3way02 Candles_NOT Animated.nif" },
	{ "norcathallsm4way01", "Nordic Cata 4way01 Candles_NOT Animated.nif" },
	{ "norcathallsm4way02", "Nordic Cata 4way02 Candles_NOT Animated.nif" },
	{ "nortmphallbgcolumnsm01", "Nordic ColumnSM01 Candles_NOT Animated.nif" },
	{ "nortmphallbgcolumnsm02", "Nordic ColumnSM02 Candles_NOT Animated.nif" },
	{ "nortmphallbgcolumn01", "Nordic Column Candles 01 03_NOT Animated.nif" },
	{ "nortmphallbgcolumn03", "Nordic Column Candles 01 03_NOT Animated.nif"}
};

static const std::vector<std::vector<std::string_view>> keywordLightGroups = {
    {"sun", "light"},   // both must be present
    {"window"},         
    {"loadscreen"}      
};

static const std::unordered_map<std::string, RE::TES_LIGHT_FLAGS> kLightFlagMap{
    { "kNone",          RE::TES_LIGHT_FLAGS::kNone },
    { "kDynamic",       RE::TES_LIGHT_FLAGS::kDynamic },
    { "kCanCarry",      RE::TES_LIGHT_FLAGS::kCanCarry },
    { "kNegative",      RE::TES_LIGHT_FLAGS::kNegative },
    { "kFlicker",       RE::TES_LIGHT_FLAGS::kFlicker },
    { "kDeepCopy",      RE::TES_LIGHT_FLAGS::kDeepCopy },
    { "kOffByDefault",  RE::TES_LIGHT_FLAGS::kOffByDefault },
    { "kFlickerSlow",   RE::TES_LIGHT_FLAGS::kFlickerSlow },
    { "kPulse",         RE::TES_LIGHT_FLAGS::kPulse },
    { "kPulseSlow",     RE::TES_LIGHT_FLAGS::kPulseSlow },
    { "kSpotlight",     RE::TES_LIGHT_FLAGS::kSpotlight },
    { "kSpotShadow",    RE::TES_LIGHT_FLAGS::kSpotShadow },
    { "kHemiShadow",    RE::TES_LIGHT_FLAGS::kHemiShadow },
    { "kOmniShadow",    RE::TES_LIGHT_FLAGS::kOmniShadow },
    { "kPortalStrict",  RE::TES_LIGHT_FLAGS::kPortalStrict },

    //  idk what ktype flag is, mabye it sets multiple flags? //kOmniShadow, kHemiShadow & kSpotShadow ??
    { "kType",          RE::TES_LIGHT_FLAGS::kType }
};

