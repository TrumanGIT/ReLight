#pragma once
#include <unordered_set>
#include <unordered_map>
#include "config.hpp"

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

extern std::map<LightConfig, std::vector<RE::NiPointer<RE::NiPointLight>>> niPointLightNodeBank;

// i dont remember what this was for, 
static const std::vector<std::pair<std::string, std::string>> childBankMap = { // not a map for priority
       { "chandel", "chandel" },                          
       { "ruins_floorcandlelampmid", "ruinsfloorcandlelampmidon" },
       { "candle", "candle" }
};

static const std::vector<std::vector<std::string_view>> keywordLightGroups = {
    {"sun", "light"},   // both must be present
    {"window"},         
    {"loadscreen"}      
};



