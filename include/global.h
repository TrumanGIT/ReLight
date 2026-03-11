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

    inline bool maxLightDistanceEnabled = false;

    inline bool enableDebugLightBulbs = false;

    inline bool enableLightFlickerPreventionMeasures = false;

    inline float lightMergeSeekingDistance = 225;

    inline float lightMergeDistance = 134;

    inline float shadowLightMergeDistance = 162;

    inline float fMaxZDiffToMerge = 28.70;

    inline float fMaxZDiffToMergeIncreased = 150.70;

    inline float g_maxShadowCompeteDistance = 314;

    inline float gMinCandleCoverage = 430;

    inline float gMinCandleCoverageSM = 300;

    //tighter restrictions for wall nodes.
    inline float minCandleCoverageWall = 285; 

    inline float gMinFireCoverageWall = 290;

    inline float gMinFireCoverage = 580.49f; 

    // larger walls shouldent have such strict bounds or no lights
    inline float maxWallSizeForStrictLightBounds = 325; 

    inline float gMinChandelierCoverage = 580; 

    inline float globalCoverage = 646;

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

    //inline std::mutex refsWithAttachedLightsMutex{};
    inline std::unordered_set<RE::FormID> refsWithAttachedLights{};

  //  inline std::mutex mergedRefsMutex{};
    inline std::unordered_set<RE::FormID> mergedRefs{};

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