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
//bool enableColorConsistency = true; 

int loggingLevel = 2; // default to info

uint8_t red = 255;
uint8_t green = 161;
uint8_t blue = 60;

RE::FormID soulCairnFormID = 0x2001408;
RE::FormID apocryphaFormID = 0x0401C0B2; 

std::vector<std::string> whitelist; // to whitelist light refs from mods by plugin name

std::vector<std::string> exclusionList; 

std::vector<std::string> exclusionListPartialMatch;

std::vector<std::string> priorityList = {};

std::map<std::string, Template> niPointLightNodeBank = {};

RE::NiPointer<RE::NiPointLight> masterNiPointLight = nullptr; 

// nodeName (lowercased) -> template mesh path
//std::unordered_map<std::string, std::string>
 //   baseMeshesAndTemplateToAttach = {};
//std::unordered_map<std::string, std::string> keywordTemplateMap = {};
