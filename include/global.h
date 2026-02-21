#pragma once
#include <unordered_set>
#include <unordered_map>
#include "config.hpp"
#include <vector>
#include <string>

namespace globals
{
    inline bool disableShadowCasters = false;
    inline bool disableTorchLights = false;
    inline bool removeFakeGlowOrbs = false;

    inline bool lastCellWasInterior = false;
    inline bool islInstalled = false;

    inline int maxLightDistance = 1000; 
    inline bool maxLightDistanceEnabled = false;

    inline bool disableLightsNotInCameraEnabled = false; 

    inline bool enableHookToRemoveLightsFromBSTriShapes = false;

    //inline bool enableLightMerging = false;

    inline float lightMergeDistance = 120; 

    inline float shadowLightMergeDistance = 162;

    inline int lightOverlapMinOnTriShapeMult = 7;

    inline int fLODFadeOutMultObjects = 9000;

    inline float frustumOverlapTolerance = 0.000001;

    inline int loggingLevel = 0;

    inline std::vector<std::string> meshPaths{};
    inline std::vector<std::string> whitelist{};
    inline std::vector<RE::BSFixedString> exclusionList{};
    inline std::vector<RE::BSFixedString> exclusionListPartialMatch{};
    inline std::vector<RE::BSFixedString> priorityList{};
    inline std::unordered_set<RE::FormID> excludedRefFormIDs{};

    inline std::unordered_set<RE::FormID> excludedLightFormIDs{};
    inline std::unordered_set<RE::FormID> baseFormsWithAttachedLights{};
    inline std::unordered_set<RE::TESObjectREFR*> refsWithAttachedLights{};


    inline RE::TESObjectLIGH* dummyLightObject = nullptr;

    inline const std::vector<std::vector<std::string_view>> keywordLightGroups = {
        { "sun", "light" },   // both must be present
        { "window" },
        { "loadscreen" },
        { "magic" },
        { "fog" },
        { "loadscreenlightmain" }
    };

}