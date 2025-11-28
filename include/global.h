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

extern std::map<LightConfig, std::vector<RE::NiPointer<RE::NiAVObject>>> niPointLightNodeBank;

// defined as static const for potential caching
static const std::vector<std::pair<std::string, std::string>> childBankMap = { // not a map for priority
       { "chandel", "chandel" },                          // handle 'chandel' substring
       { "ruins_floorcandlelampmid", "ruinsfloorcandlelampmidon" },
       { "candle", "candle" }
};

static const std::unordered_set<std::string> nordicHallMeshes = {
    "norcathallsm1way01",
    "norcathallsm1way02",
    "norcathallsm1way03",
    "norcathallsm2way01",
    "norcathallsm3way01",
    "norcathallsm3way02",
    "norcathallsm4way01",
    "norcathallsm4way02",
    "nortmphallbgcolumnsm01",
    "nortmphallbgcolumnsm02",
    "nortmphallbgcolumn01",
    "nortmphallbgcolumn03"
};

static const std::vector<std::vector<std::string_view>> keywordLightGroups = {
    {"sun", "light"},   // both must be present
    {"window"},         
    {"loadscreen"}      
};



