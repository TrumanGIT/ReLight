#pragma once
#include <unordered_set>
#include <unordered_map>
#include "config.hpp"
#include <vector>
#include <string>


namespace globals

{
    inline std::atomic_bool unDesiredMenuOpen = false;


    inline uint32_t nextID = 1;

    inline bool disableShadowCasters = false;
    inline bool disableTorchLights = false;
    inline bool removeFakeGlowOrbs = false;

    inline bool lastCellWasInterior = false;
    inline bool currentCellIsInterior = false;

    inline bool islInstalled = false;

    inline bool enableDebugLightBulbs = false;

    //Flicker Prevention
    inline bool enableLightFlickerPreventionMeasures = false;

    inline std::atomic_bool cellFullyLoaded = false;

    inline std::atomic_bool secondAfterCellFullyLoaded = false;

    inline std::chrono::steady_clock::time_point cellFullyLoadedTimerStart;

    // Merge

    inline int lightMergeMaxLights = 10;

    inline float lightMergeSeekingDistance = 188;

    inline float lightMergeDistance = 130;

    inline float fMaxZDiffToMergeIncreased = 160.70f;

    inline float shadowLightMergeDistance = 162;

    inline float fMaxZDiffToMerge = 28.70f;

    inline float lightFadePerMerge = 0.3f;   

    inline float lightRadiusPerMerge = 0.2f; 

    inline float lightFadeMax = 2.0f;    // maximum total fade multiplier
    inline float lightRadiusMax = 2.0f;  // maximum total radius multiplier

    // flicker prevention

    // Candle Bounds
    inline float minCandleCoverageSM = 383;

    inline float minCandleCoverage = 470;

  //  inline float minCandleCoverageXL = 1100;

    //FireBounds
    inline float minFireCoverageXL = 1200;

    inline float minFireCoverage = 680.49f;

    // Chandeliers
  //  inline float minChandelierCoverageSM = 314;

    inline float minChandelierCoverage = 630;

    inline float globalCoverage = 700; 

    // used to reinitialize lights based on this distance as we put lights into object geomatry
    inline float fLODFadeOutMultObjects = 9000;

    inline int loggingLevel = 0;

    inline std::vector<std::string> meshPaths{};
    inline std::vector<std::string> whitelist{};
    inline std::vector<std::string> meshPathExclusionList{};
    inline std::vector<std::string> meshPathExclusionListPartialMatch{};
    inline std::vector<std::string> priorityList{};

    inline std::unordered_set<RE::FormID> excludedRefFormIDs{};

    inline std::unordered_set<RE::FormID> excludedLightFormIDs{};
    inline std::unordered_set<RE::FormID> baseFormsWithAttachedLights{};


    inline std::mutex refsWithAttachedLightsMutex{};
    inline std::unordered_set<RE::FormID> refsWithAttachedLights{};

      inline std::mutex mergedRefsMutex{};
    inline std::unordered_set<RE::FormID> mergedRefs{};

    //I tag wall meshes in load3d hook so I can find them faster in islightaffectingsurface hook.
   // inline std::unordered_set<RE::FormID> wallMeshes{};

    inline std::vector<std::string> keywordLightGroups = {};

}