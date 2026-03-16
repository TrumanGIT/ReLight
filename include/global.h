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

    inline bool enableDebugLightBulbs = false;

    //Flicker Prevntion
    inline bool enableLightFlickerPreventionMeasures = false;

    //inline uint32_t flickerPreventionLightLimit = 6;

    inline std::atomic_bool cellFullyLoaded = false;

    // Merge

    inline int lightMergeMaxLights = 10;

    inline float lightMergeSeekingDistance = 225;

    inline float lightMergeDistance = 147;

    inline float lightFadePerMerge = 0.3f;   

    inline float lightRadiusPerMerge = 0.2f; 

    inline float lightFadeMax = 2.0f;    // maximum total fade multiplier
    inline float lightRadiusMax = 2.0f;  // maximum total radius multiplier

    //used when detecs a ruin candle
    inline float fMaxZDiffToMergeIncreased = 160.70f;

    inline float shadowLightMergeDistance = 162;

    inline float fMaxZDiffToMerge = 28.70f;

    // Candle Bounds
    inline float minCandleCoverage = 430;

    inline float minCandleCoverageSM = 383;

    inline float minCandleCoverageWall = 210;

    //FireBounds
    inline float minFireCoverageWall = 465;

    inline float minFireCoverage = 580.49f;

    // Chandeliers
    inline float minChandelierCoverage = 580;

    inline float maxShadowCompeteDistance = 314;

    inline float globalCoverage = 646;

    // larger walls shouldent have such strict bounds or no lights
    inline float maxWallSizeForStrictLightBounds = 325;

    // used to reinitialize lights based on this distance as we put lights into object geomatry
    inline float fLODFadeOutMultObjects = 9000;

    //Is Light Attached to Surface
    //inline std::atomic_bool staredtIsLightAffectingSurfaceTimer = false;

  // inline std::chrono::steady_clock::time_point lightEvalStart;

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
        { "loadscreen" },
        {"fungus"}
    };

}