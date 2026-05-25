#pragma once
#include <unordered_set>
#include <unordered_map>
#include "config.hpp"
#include <vector>
#include <string>


namespace globals

{
    inline float brightnessModifier = 1.0f; 
    //Attach Misc Lights
    inline std::atomic_bool unDesiredMenuOpen = false;

    inline std::atomic_bool magicLightQueued = false;

    inline RE::NiAVObject* magicLightAttachNode = nullptr; 

    inline std::vector<RE::NiAVObject*> torchLightAttachNodes{};

    inline uint32_t nextID = 1;

    inline bool disableGameLights = true;
 
    inline bool removeFakeGlowOrbs = false;

    inline bool lastCellWasInterior = false;
    inline bool currentCellIsInterior = false;

    inline bool islInstalled = false;
    inline bool disableISL = false; 

    inline bool enableDebugLightBulbs = false;

    //Flicker Prevention
    inline bool enableLightFlickerPreventionMeasures = false;

    inline std::atomic_bool cellFullyLoaded = false;

    inline std::atomic_bool secondAfterCellFullyLoaded = false;

    inline std::chrono::steady_clock::time_point cellFullyLoadedTimerStart;

    // any surface (trishape world bounds) larger will not be affected by light flicker prevention
    inline int largeSurfaceSize = 1800; 

    // below coverage values will not affect surfaces (trishape world bounds)  larger then this value
    inline int mediumSurfaceSize = 700;

    // used only to limit how many candles can be on a small surface (trishape world bounds) 
    inline int smallSurfaceSize = 350;

    //fires
    inline int maxFiresPerSurfaceSM = 4;
    inline int maxFiresPerSurfaceM = 4;

    //candles
    inline int maxCandlesPerSurfaceSM = 4;
    inline int maxCandlesPerSurfaceM = 6;

    // candles cant affect surfaces this far below them enforced in islightaffectingsurface hook
    inline float maxCandleZDistance = 200;
    inline float maxCandleDistance = 470;

    // Chandeliers
    inline int maxChandeliersPerSurfaceSM = 3;
    inline int maxChandeliersPerSurfaceM = 3;
    inline float maxChandelierDistance = 630;

    // chandeliers cant affect surfaces this far below them enforced in islightaffectingsurface hook
    inline float maxChandelierZDistance= 350;

    // Merge

    inline bool enableLightMerging = true;

    inline int lightMergeMaxLights = 12;

    inline float lightMergeSeekingDistance = 188;

    inline float lightMergeDistance = 130;

    inline float fMaxZDiffToMergeIncreased = 160.70f;

    inline float shadowLightMergeDistance = 162;

    inline float fMaxZDiffToMerge = 28.70f;

    inline float lightFadePerMerge = 0.3f;   

    inline float lightRadiusPerMerge = 0.2f; 

    inline float lightFadeMax = 2.0f;    // maximum total fade multiplier
    inline float lightRadiusMax = 2.0f;  // maximum total radius multiplier


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

    inline std::vector<std::string> enableByEditorID = {};

    inline std::vector<std::string> disableByEditorID = {};

}