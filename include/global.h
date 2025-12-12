#pragma once
#include <unordered_set>
#include <unordered_map>
#include "config.hpp"

extern bool disableShadowCasters;
extern bool disableTorchLights;
extern bool removeFakeGlowOrbs;

//extern bool enableColorConsistency;

extern int loggingLevel;

extern uint8_t red;
extern uint8_t green;
extern uint8_t blue;

//extern RE::TESObjectLIGH* loadScreenLightMain;

extern RE::FormID soulCairnFormID;

extern RE::FormID apocryphaFormID;

extern std::vector<std::string> whitelist;

extern std::vector<std::string> exclusionList;

extern std::vector<std::string> exclusionListPartialMatch;

extern std::unordered_set<RE::FormID> excludedLightFormIDs;

extern std::vector<std::string> priorityList;

extern std::map<std::string, Template> niPointLightNodeBank;

static const std::vector<std::vector<std::string_view>> keywordLightGroups = {
    {"sun", "light"},   // both must be present
    {"window"},         
    {"loadscreen"},
    {"magic"}
};



