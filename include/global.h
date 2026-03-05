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
    inline bool currentCellIsInterior = false;

    inline bool islInstalled = false;

    inline int maxLightDistance = 1000;
    inline bool maxLightDistanceEnabled = false;

    inline bool enableLightFlickerPreventionMeasures = false;

    inline float lightMergeDistance = 130;

    inline float shadowLightMergeDistance = 162;

    inline float fMaxZDiffToMerge = 25;

    inline float g_maxShadowCompeteDistance = 314;

    inline float gMinCandleCoverage = 430;

    //tighter restrictions for wall nodes.
    inline float minCandleCoverageWall = 210; 

    inline float gMinFireCoverageWall = 140; 

    inline float gMinFireCoverage = 580.49f; 

    // larger walls shouldent have such strict bounds or no lights
    inline float maxWallSizeForStrictLightBounds = 325; 

    inline float gMinChandelierCoverage = 580; 

    inline float globalCoverage = 580;

    inline float fLODFadeOutMultObjects = 9000;

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

    //I tag wall meshes in load3d hook so I can find them faster in islightaffectingsurface hook.
    inline std::unordered_set<RE::FormID> wallMeshes{};

    inline const std::vector<std::vector<std::string_view>> keywordLightGroups = {
        { "sun"},   
        { "window" },
        { "loadscreen" },
        { "magic" },
        { "fog" },
        { "loadscreenlightmain" }
    };

}