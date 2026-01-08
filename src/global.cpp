#include "global.h"
#include <map>
#include <array>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>


bool disableShadowCasters = true;
bool disableTorchLights = true;
bool removeFakeGlowOrbs = true;

int loggingLevel = 2; // default to info

RE::FormID soulCairnFormID = 0x2001408;
RE::FormID apocryphaFormID = 0x0401C0B2; 

std::vector<std::string> whitelist; // to whitelist light refs from mods by plugin name

std::vector<std::string> exclusionList; 

std::vector<std::string> exclusionListPartialMatch;

std::vector<std::string> priorityList = {};

std::map<std::string, LightConfig> niPointLightNodeBank = {};

RE::NiPointer<RE::NiPointLight> masterNiPointLight = nullptr; 

RE::TESObjectLIGH* dummyLightObject = nullptr;

// nodeName (lowercased) -> template mesh path
//std::unordered_map<std::string, std::string>
 //   baseMeshesAndTemplateToAttach = {};
//std::unordered_map<std::string, std::string> keywordTemplateMap = {};
