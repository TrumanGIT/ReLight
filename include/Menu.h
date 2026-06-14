#pragma once
#include "SKSEMenuFramework.h"
#include "logger.hpp"
#include "LightData.h"
#include "Utility.h"
#include "LightManager.h"

namespace UI {

    struct LightGroupData
    {
        // All lights in the scene, keyed by their index in the global `lights` vector.
        std::unordered_map<std::string, std::vector<int>> byConfigPath;

        // configPath groups that belong to a named menuCategory.
        std::unordered_map<std::string, std::vector<std::string>> byCategory;

        // configPath groups with no menuCategory.
        std::vector<std::string> uncategorized;

        // configPaths in alphabetical display order (by resolved menu name).
        std::vector<std::string> sortedConfigPaths;

        // How many lights share the same resolved menu name (used to disambiguate
        // with a mesh path suffix).
        std::unordered_map<std::string, int> menuNameCounts;
    };

    void Register();
    void __stdcall RenderSettings();
    void __stdcall RenderLightEditor();
    void __stdcall  RenderLightFlickerPreventionMenu();
    void __stdcall  RenderLightMergeMenu();
    void __stdcall RenderAttachRemove();
    inline MENU_WINDOW reLightMenuWindow;

    inline void debugLogAllLights() {
        auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
        if (!ssNode) {
            logger::warn("ShadowSceneNode[0] is null!");
            return;
        }

        auto& rt = ssNode->GetRuntimeData();

        for (auto& light : rt.activeLights) {
            if (!light) continue;

            std::string lightName = light->light->name.c_str();

            if (lightName[0] != 'R' || lightName[1] != 'L')
                continue;

            const auto& lightRt = light->light->GetLightRuntimeData();
            auto it = LightData::configIDToJsonCfg.find(lightRt.unk138);
            if (it == LightData::configIDToJsonCfg.end()) {
                logger::warn("Config ID {:08X} not found in config map", lightRt.unk138);
                continue;  // or continue depending on your function
            }

            const auto& cfg = it->second;

            logger::debug(
                "[Light] '{}'\n"
                "  brightness        {}\n"
                "  startingBrightness{}\n"
                "  radius            {}\n"
                "  flickerIntensity  {}\n"
                "  flickersPerSecond {}\n"
                "  worldPos          {}\n"
                "  unk060            {}\n"
                "  configID          {}",
                lightName,
                lightRt.fade,
                cfg.startingFade,
                lightRt.radius,
                cfg.flickerIntensity,
                cfg.flickersPerSecond,
                light->light->world.translate,
                light->unk060,
                cfg.configID
            );

            if (globals::islInstalled) {

                auto* islRt = Overlay::Get(light->light.get());

                if (!islRt)
                    return;

                logger::debug("Light size {} light cuttoff {}", islRt->size, islRt->cutoffOverride);

            }
        }

        for (auto& light : rt.activeShadowLights) {
            if (!light) continue;

            std::string lightName = light->light->name.c_str();

            if (lightName[0] != 'R' || lightName[1] != 'L')
                continue;

            const auto& lightRt = light->light->GetLightRuntimeData();

            auto it = LightData::configIDToJsonCfg.find(lightRt.unk138);
            if (it == LightData::configIDToJsonCfg.end()) {
                logger::warn("Config ID {:08X} not found in config map", lightRt.unk138);
                continue;
            }

            const auto& cfg = it->second;

            logger::debug(
                "[Shadow Light] '{}'\n"
                "  brightness        {}\n"
                "  startingBrightness{}\n"
                "  radius            {}\n"
                "  flickerIntensity  {}\n"
                "  flickersPerSecond {}\n"
                "  worldPos          {}\n"
                "  unk060            {}\n"
                "  configID          {}",
                lightName,
                lightRt.fade,
                cfg.startingFade,
                lightRt.radius,
                cfg.flickerIntensity,
                cfg.flickersPerSecond,
                light->light->world.translate,
                light->unk060,
                cfg.configID
            );


            if (globals::islInstalled) {

                auto* islRt = Overlay::Get(light->light.get());

                if (!islRt)
                    return;

                logger::debug("Shadow Light size {} light cuttoff {}", islRt->size, islRt->cutoffOverride);

            }
        }

        logger::debug("printing all refs in attached lights set");
        for (const auto formID : globals::refsWithAttachedLights) {


            logger::debug("ref {:08X}", formID);

        }
    }

    // RenderLightEditor FUNCTIONS BELOW
    ////////////////////////////////////////

   inline bool saveSettingsToIni()
    {
        logger::info("Saving ReLight.ini...");
        const std::string path = "Data\\SKSE\\Plugins\\ReLight.ini";

        // READ: grab everything from the exclude refs section downward as a raw block
        std::string preservedBlock;
        {
            std::ifstream inFile(path);
            if (inFile.is_open())
            {
                std::string line;
                bool inSection = false;
                while (std::getline(inFile, line))
                {
                    if (!inSection && line.find("; add esps by name") != std::string::npos)
                        inSection = true;
                    if (inSection)
                        preservedBlock += line + "\n";
                }
            }
        }

        std::ofstream outFile(path, std::ios::trunc);
        if (!outFile.is_open())
        {
            logger::error("Failed to open {} for writing!", path);
            return false;
        }
        outFile << ";allow relight to disable all non relight lights(to start with a clean base)\n";
        outFile << "disableGameLights=" << (globals::disableGameLights ? "true" : "false") << "\n\n";
        outFile << "; enable light flicker prevention (Not for CS users)\n";
        outFile << "enableLightFlickerPrevention=" << (globals::enableLightFlickerPreventionMeasures ? "true" : "false") << "\n\n";
        outFile << ";Change brightness of all Relight lights\n";
        outFile << "lightBrightnessMultiplier=" << std::clamp(globals::brightnessModifier, 0.1f, 2.0f) << "\n\n";
        outFile << ";Change brightness of all non SKSE lights\n";
        outFile << "nonSKSELightsBrightnessMultiplier=" << std::clamp(globals::vanillaBrightnessModifier, 0.1f, 2.0f) << "\n\n";
        outFile << "; remove fake glow orbs (default = true)\n";
        outFile << "removeFakeGlowOrbs=" << (globals::removeFakeGlowOrbs ? "true" : "false") << "\n\n";
        outFile << "; enable debug bulbs (default = false)\n";
        outFile << "enableDebugBulbs=" << (globals::enableDebugLightBulbs ? "true" : "false") << "\n\n";
        outFile << "; enable debug lines (default = true)\n";
        outFile << "enableDebugLines=" << (globals::enableDebugLines ? "true" : "false") << "\n\n";
        outFile << "; max distance for debug lines (default = false)\n";
        outFile << "maxDistanceForDrawDebuglines=" << globals::distanceForDrawDebugLines << "\n\n";
        outFile << "; disable Inverse Squared Lighting (relight lights and menu will change to vanilla, Only works if ISL is disabled at boot in CS settings)\n";
        outFile << "disableISL=" << (globals::disableISL ? "true" : "false") << "\n\n";
        outFile << "; Logging Level (0: critical, 1: warnings/errors, 2: info, 3: debug)\n";
        outFile << "loggingLevel=" << globals::loggingLevel << "\n";
        outFile << "\n; Light merge settings\n\n";
        outFile << "enableLightMerging="<< (globals::enableLightMerging ? "true" : "false") << "\n";
        outFile << "light merge distance=" << globals::lightMergeDistance << "\n";
        outFile << "shadow light merge distance=" << globals::shadowLightMergeDistance << "\n";
        outFile << "light merge distance increased=" << globals::lightMergeSeekingDistance << "\n";
        outFile << "max z diff to merge=" << globals::fMaxZDiffToMerge << "\n";
        outFile << "max z diff to merge increased=" << globals::fMaxZDiffToMergeIncreased << "\n";
        outFile << "light fade increase per merge=" << globals::lightFadePerMerge << "\n";
        outFile << "light radius increase per merge=" << globals::lightRadiusPerMerge << "\n";
        outFile << "light fade max=" << globals::lightFadeMax << "\n";
        outFile << "light radius max=" << globals::lightRadiusMax << "\n";
        outFile << "light merge max lights=" << globals::lightMergeMaxLights << "\n\n";
        outFile << "\n;Light FlickerPrevention Settings\n\n";
        outFile << ";trishapes with worldbound radius larger than this number will not participate in light flicker prevention\n";
        outFile << "large surface size=" << globals::largeSurfaceSize << "\n";
        outFile << ";trishapes with worldbound radius larger then this number will not participate in light type distance checks\n";
        outFile << "medium surface size=" << globals::mediumSurfaceSize << "\n";
        outFile << ";trishapes with worldbound radius larger then this number will not participate in max light types per surface checks\n";
        outFile << "small surface size=" << globals::smallSurfaceSize << "\n";
        outFile << "max candles per sm surface=" << globals::maxCandlesPerSurfaceSM << "\n";
        outFile << "max chandeliers per sm surface=" << globals::maxChandeliersPerSurfaceSM << "\n";
        outFile << "max fires per sm surface=" << globals::maxFiresPerSurfaceSM << "\n";
        outFile << "max candles per m surface=" << globals::maxCandlesPerSurfaceM << "\n";
        outFile << "max chandeliers per m surface=" << globals::maxChandeliersPerSurfaceM << "\n";
        outFile << "max fires per m surface=" << globals::maxFiresPerSurfaceM << "\n";
        outFile << "max candle distance=" << globals::maxCandleDistance << "\n";
        outFile << "max candle z distance=" << globals::maxCandleZDistance << "\n";
        outFile << "max chandelier distance=" << globals::maxChandelierDistance << "\n";
        outFile << "max chandelier z distance=" << globals::maxChandelierZDistance << "\n";

        // dump the entire preserved block back verbatim - comments, formids, everything
        if (!preservedBlock.empty())
            outFile << "\n" << preservedBlock;

        outFile.close();
        logger::info("ReLight.ini saved successfully!");
        return true;
   }

   inline void restoreLightToDefaults(RE::NiPointer<RE::NiLight> light) {
        if (!light) {
            logger::warn("Selected light is null, cannot restore defaults");
            return;
        }

        const std::string lightName = light->name.c_str();

        LightConfig backupCfg;

        auto itDefault = LightData::defaultConfigs.find(light->unk138);
        if (itDefault == LightData::defaultConfigs.end()) {
            logger::warn("no config found to restore defaults from");
            return;
        }
        else {
            backupCfg = itDefault->second;
        }

        auto& lightData = light->GetLightRuntimeData();

        auto itCfg = LightData::configIDToJsonCfg.find(lightData.unk138);
        if (itCfg == LightData::configIDToJsonCfg.end()) {
            logger::warn("No JSON config entry found for light ID {}", lightData.unk138);
            return;
        }
        auto& cfg = itCfg->second;

        cfg.position = backupCfg.position;


        auto ref = light->GetUserData();

        if (!ref) {
            logger::warn("no ref for configID {} cant restore defaults", light->unk138);
            return;
        }

        lightData.radius = LightData::getNiPointLightRadius(backupCfg, ref->GetScale());
        lightData.fade = backupCfg.brightness;
        LightData::setNiPointLightAmbientAndDiffuse(light.get(), backupCfg);
        LightData::setNiPointLightPos(light.get(), backupCfg, ref->GetScale());

        //update the parent or sometimes it doesent work.
        if (auto* parent = light->parent) {
            RE::NiUpdateData updateData{};
            updateData.time = 0.0f;
            updateData.flags = RE::NiUpdateData::Flag::kDirty;
            parent->UpdateTransformAndBounds(updateData);
        }

        cfg.startingFade = backupCfg.startingFade;
        cfg.flickerIntensity = backupCfg.flickerIntensity;
        cfg.flickersPerSecond = backupCfg.flickersPerSecond;
        cfg.flickerAmplitude = backupCfg.flickerAmplitude;
        cfg.flags = backupCfg.flags;

        // Propagate to active lights in the shader node
        auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
        if (!ssNode) return;

        auto& rt = ssNode->GetRuntimeData();

        auto updateLightList = [&](auto& lightList) {
            for (auto& currentLight : lightList) {
                if (!currentLight) continue;

                auto& activeData = currentLight->light->GetLightRuntimeData();

                if (activeData.unk138 == lightData.unk138) {
                    activeData = lightData;
                    currentLight->light->local.translate = light->local.translate;

                    if (globals::islInstalled) {
                        if (auto* isl = Overlay::Get(currentLight->light.get())) {
                            isl->cutoffOverride = backupCfg.cutoffOverride;
                            isl->size = backupCfg.size;
                        }
                    }
                }
            }
            };
        updateLightList(rt.activeLights);
        updateLightList(rt.activeShadowLights);

        logger::info("Restored '{}' to default config", lightName);
    }


   inline bool compareLightNames(const char* a, const char* b) {
       if (!a) a = "";
       if (!b) b = "";
       for (;; ++a, ++b) {
           unsigned char ca = (unsigned char)std::tolower((unsigned char)*a);
           unsigned char cb = (unsigned char)std::tolower((unsigned char)*b);
           if (ca < cb) return true;
           if (ca > cb) return false;
           if (ca == 0) return false;
       }
   }

   //TODO:: clean and only use isl overlay if its installed
   inline void getAllLights(std::vector<RE::NiPointer<RE::BSLight>>& lights, bool& lightAlreadyInList) {
       auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
       if (!ssNode) {
           logger::warn("ShadowSceneNode[0] is null!");
           return;
       }

       auto& rt = ssNode->GetRuntimeData();

       for (auto& light : rt.activeLights) {
           if (!light) continue;

           std::string lightName = light->light->name.c_str();

           if (lightName[0] != 'R' || lightName[1] != 'L')
               continue;

           const auto& currentRt = light->light->GetLightRuntimeData();

           for (auto& existingLight : lights) {

               if (!existingLight) continue;
               // unk138 is a config id in this case, do this to handle editing multiple lights to 1 mesh 
               if (existingLight->light->unk138 == currentRt.unk138) {
                   // Light already exists in the list, skip adding
                   lightAlreadyInList = true;
                   break;
               }
           }

           if (!lightAlreadyInList) {

               lights.push_back(light);
           }

           lightAlreadyInList = false;
       }

       for (auto& shadowLight : rt.activeShadowLights) {
           if (!shadowLight) continue;

           std::string lightName = shadowLight->light->name.c_str();

           if (lightName[0] != 'R' || lightName[1] != 'L')
               continue;

           const auto& currentRt = shadowLight->light->GetLightRuntimeData();

           //const auto& cfg = LightData::configIDToJsonCfg[currentRt.unk138];

           bool shadowLightAlreadyInList = false;
           for (auto& existingLight : lights) {
               if (existingLight->light->unk138 == currentRt.unk138) {
                   shadowLightAlreadyInList = true;
                   break;
               }
           }

           if (!shadowLightAlreadyInList) {
               lights.push_back(shadowLight);
           }
       }
   }

    inline int getLightKey(const RE::NiPointer<RE::BSLight>& l) {
        if (!l || !l->light) return -1;
        return l->light->GetLightRuntimeData().unk138;  // runtime configID key
    }

    inline void refreshLight(
        const RE::NiPointer<RE::BSLight>& activeLight,
        std::vector<RE::NiPointer<RE::BSLight>>& refreshedLights,
        std::unordered_map<int, int>& keyToIndex,
        std::unordered_set<int>& seen) {

        if (!activeLight || !activeLight->light) return;

        const char* name = activeLight->light->name.c_str();
        if (!name || name[0] != 'R' || name[1] != 'L')
            return;

        int key = activeLight->light->GetLightRuntimeData().unk138;
        if (key < 0) return;

        seen.insert(key);

        auto it = LightData::configIDToJsonCfg.find(key);
        if (it != LightData::configIDToJsonCfg.end()) {
        }
        else {
            logger::debug("light :{} (key={}) has no json cfg entry", name, key);
        }

             // Refresh light pointer in list or add if new
        auto itIdx = keyToIndex.find(key);
        if (itIdx == keyToIndex.end()) {
            refreshedLights.push_back(activeLight);
            keyToIndex[key] = (int)refreshedLights.size() - 1;
        }
        else {
            refreshedLights[itIdx->second] = activeLight;
        }
    }

    inline void refreshAllLights(int& selectedIndex, std::vector<RE::NiPointer<RE::BSLight>>& lights) {

        auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
        if (!ssNode) {
            logger::warn("ShadowSceneNode[0] is null!");
            return;
        }

        auto& rt = ssNode->GetRuntimeData();

        int selectedKey = -1;
        if (selectedIndex >= 0 && selectedIndex < (int)lights.size()) {
            selectedKey = getLightKey(lights[selectedIndex]);
        }

        // Build light indices map
        std::unordered_map<int, int> keyToIndex;
        keyToIndex.reserve(lights.size());
        for (int i = 0; i < (int)lights.size(); ++i) {
            int key = getLightKey(lights[i]);
            if (key >= 0) keyToIndex[key] = i;
        }

        // For tracking seen keys
        std::unordered_set<int> seen;
        seen.reserve(256);

        for (auto& activeLight : rt.activeLights) {
            refreshLight(activeLight, lights, keyToIndex, seen);
        }
        for (auto& activeShadowLight : rt.activeShadowLights) {
            refreshLight(activeShadowLight, lights, keyToIndex, seen);
        }

        // Removes non-active lights from the list
        lights.erase(std::remove_if(lights.begin(), lights.end(),
            [&](const RE::NiPointer<RE::BSLight>& l) {
                int key = getLightKey(l);
                return key < 0 || (seen.find(key) == seen.end());
            }),
            lights.end());


        // Update selectded index after sorting
        selectedIndex = -1;
        if (selectedKey >= 0) {
            for (int i = 0; i < (int)lights.size(); ++i) {
                if (getLightKey(lights[i]) == selectedKey) {
                    selectedIndex = i;
                    break;
                }
            }
        }
    }

    // timing is a real issue, ive tried 4 or 5 different ways anjd lights dont intiialize 
    // properly and this was the way that i ended up on
    inline void RefreshNonRuntimeSettings(LightConfig cfg)
    {

        auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
        if (!ssNode) {
            logger::warn("ShadowSceneNode[0] is null cant reinitialize lights");
            return;
        }

        auto addLight = [&](const RE::NiPointer<RE::NiLight>& light) {
            if (!light)
                return;

            auto ref = light->GetUserData();
            if (!ref) {
                logger::warn("no ref for configID {} cant refresh non runtime settings", light->unk138);
                return;
            }

            // delay
            SKSE::GetTaskInterface()->AddTask([cfg, ref]() mutable {

                const RE::FormID formID = ref->GetFormID();

                auto loadedModel = ref->Get3D();
                if (!loadedModel) {
                    logger::error("no loaded model for ref, cant refresh non runtime settings");
                    return;
                }

                RE::NiNode* root = loadedModel->AsNode();
                if (!root) {
                    logger::error("no attachable node for ref cant refresh non runtime settings");
                    return;
                }

                auto baseObj = ref->GetBaseObject();
                if (!baseObj) {
                    logger::error("no base object for ref cant refresh non runtime settings");
                    return;
                }

                const auto bm = baseObj->As<RE::TESModel>();
                if (!bm) {
                    logger::error("base object is not TESModel cant refresh non runtime settings");
                    return;
                }

                auto currentModel = std::string(bm->GetModel());

                auto meshName = extractMeshName(currentModel);

                bool attachedDebugMarker = false;

                RE::NiLight* niLight = LightManager::AttachLight(
                    cfg,
                    root,
                    ref,
                    meshName,
                    formID,
                    attachedDebugMarker);

                if (!niLight) {
                    logger::error("no niLight cant refresh non runtime settings");
                    return;
                }

                UpdateRefRootTransforms(ref);

                });

            };

        for (const auto& l : ssNode->activeLights) {
            if (!l || !l->light)
                continue;

            //skip non relight lights
            auto name = std::string_view(l->light->name.c_str());
            if (name.size() < 2 || name[0] != 'R' || name[1] != 'L')
                continue;

            // find config of current light
            auto it2 = LightData::configIDToJsonCfg.find(l->light->unk138);

            if (it2 == LightData::configIDToJsonCfg.end()) {
                logger::warn("configID {} not found in config map cant refresh lights", cfg.configID);
                return;
            }

            //if current light does not == selected light config path and json index, skip
            // this is because multiple configs can have same json index and config path but have different config ids.
            if (it2->second.configPath != cfg.configPath || it2->second.jsonIndex != cfg.jsonIndex)
                continue;

            // remove any matches 
            ssNode->RemoveLight(l->light.get());

            // add anew light
            addLight(l->light);
        }

        for (const auto& l : ssNode->activeShadowLights) {
            if (!l || !l->light)
                continue;

            auto name = std::string_view(l->light->name.c_str());
            if (name.size() < 2 || name[0] != 'R' || name[1] != 'L')
                continue;

            auto it3 = LightData::configIDToJsonCfg.find(l->light->unk138);

            if (it3 == LightData::configIDToJsonCfg.end()) {
                logger::warn("configID {} not found in config map cant refresh lights", cfg.configID);
                return;
            }

            if (it3->second.configPath != cfg.configPath || it3->second.jsonIndex != cfg.jsonIndex)
                continue;

            ssNode->RemoveLight(l->light.get());

            addLight(l->light);
        }

        // reset tri light cache otherwise light flcker prevention mode wont let the light on.
        globals::secondAfterCellFullyLoaded.store(false);
        LightData::ResetTriLightCache();
    }


    inline std::string ResolveMenuName(const LightConfig& cfg)
    {
        if (!cfg.menuName.empty())
            return cfg.menuName;
        if (!cfg.meshPaths.empty())
            return cfg.meshPaths[0];
        return "Unknown Light";
    }

    inline bool DeleteSelectedLightTemplate(int& selectedIndex, std::vector<RE::NiPointer<RE::BSLight>>& lights)
    {
        if (selectedIndex < 0 || selectedIndex >= lights.size())
            return false;

        auto selectedLight = lights[selectedIndex];

        if (!selectedLight || !selectedLight->light)
            return false;

        auto niLight = selectedLight->light.get();

        auto it = LightData::configIDToJsonCfg.find(niLight->unk138);
        if (it == LightData::configIDToJsonCfg.end())
            return false;

        const std::string deletedPath = it->second.configPath;

        if (deletedPath.empty())
            return false;

        if (!std::filesystem::remove(deletedPath))
            return false;

        auto removeByConfigPath = [&](auto& map)
            {
                for (auto itMap = map.begin(); itMap != map.end();) {
                    auto& vec = itMap->second;

                    vec.erase(
                        std::remove_if(
                            vec.begin(),
                            vec.end(),
                            [&](const LightConfig& c)
                            {
                                return c.configPath == deletedPath;
                            }),
                        vec.end());

                    if (vec.empty()) {
                        itMap = map.erase(itMap);
                    }
                    else {
                        ++itMap;
                    }
                }
            };

        for (auto itCfg = LightData::configIDToJsonCfg.begin();
            itCfg != LightData::configIDToJsonCfg.end();)
        {
            if (itCfg->second.configPath == deletedPath) {
                itCfg = LightData::configIDToJsonCfg.erase(itCfg);
            }
            else {
                ++itCfg;
            }
        }

        removeByConfigPath(LightData::meshPathToJsonCfg);
        removeByConfigPath(LightData::meshPathToJsonCfgExteriors);
        removeByConfigPath(LightData::refFormIDToJsonCfg);
        removeByConfigPath(LightData::refFormIDToJsonCfgExteriors);

        logger::info("Deleted light template '{}'", deletedPath);

        selectedIndex = -1;

        return true;
    }

    inline LightGroupData BuildCatagorizedLights(
        const std::vector<RE::NiPointer<RE::BSLight>>& lights)
    {
        LightGroupData out;

        // --- group indices by config path -----------------------------------
        for (int i = 0; i < static_cast<int>(lights.size()); ++i)
        {
            auto& light = lights[i];
            if (!light || !light->light)
                continue;

            auto configID = light->light->GetLightRuntimeData().unk138;
            auto it = LightData::configIDToJsonCfg.find(configID);
            if (it == LightData::configIDToJsonCfg.end())
                continue;

            const std::string& path =
                it->second.configPath.empty() ? "Unknown Config"
                : it->second.configPath;

            out.byConfigPath[path].push_back(i);
        }

        // --- count duplicate menu names -------------------------------------
        for (auto& [configPath, indices] : out.byConfigPath)
        {
            if (indices.empty())
                continue;

            auto& firstLight = lights[indices.front()];
            if (!firstLight || !firstLight->light)
                continue;

            auto configID = firstLight->light->GetLightRuntimeData().unk138;
            auto it = LightData::configIDToJsonCfg.find(configID);
            if (it == LightData::configIDToJsonCfg.end())
                continue;

            out.menuNameCounts[ResolveMenuName(it->second)]++;
        }

        // --- sort config paths by resolved display name ---------------------
        for (auto& [path, _] : out.byConfigPath)
            out.sortedConfigPaths.push_back(path);

        std::sort(
            out.sortedConfigPaths.begin(),
            out.sortedConfigPaths.end(),
            [&](const std::string& a, const std::string& b)
            {
                auto ResolveName = [&](const std::string& configPath) -> std::string
                    {
                        auto& indices = out.byConfigPath[configPath];
                        if (indices.empty())
                            return {};

                        auto& light = lights[indices.front()];
                        if (!light || !light->light)
                            return {};

                        auto configID = light->light->GetLightRuntimeData().unk138;
                        auto it = LightData::configIDToJsonCfg.find(configID);
                        if (it == LightData::configIDToJsonCfg.end())
                            return {};

                        return ResolveMenuName(it->second);
                    };

                return compareLightNames(
                    ResolveName(a).c_str(),
                    ResolveName(b).c_str());
            });

        // --- bucket into categories -----------------------------------------
        for (const auto& configPath : out.sortedConfigPaths)
        {
            auto& indices = out.byConfigPath[configPath];
            if (indices.empty())
                continue;

            auto& light = lights[indices.front()];
            if (!light || !light->light)
                continue;

            auto configID = light->light->GetLightRuntimeData().unk138;
            auto it = LightData::configIDToJsonCfg.find(configID);
            if (it == LightData::configIDToJsonCfg.end())
                continue;

            const std::string& category = it->second.menuCategory;
            if (category.empty())
                out.uncategorized.push_back(configPath);
            else
                out.byCategory[category].push_back(configPath);
        }

        return out;
    }

    inline void DrawCatagoryGroup(
        const std::string& configPath,
        const LightGroupData& groupData,
        const std::vector<RE::NiPointer<RE::BSLight>>& lights,
        int& selectedIndex)
    {
        auto pathIt = groupData.byConfigPath.find(configPath);
        if (pathIt == groupData.byConfigPath.end() || pathIt->second.empty())
            return;

        const auto& indices = pathIt->second;
        int          firstIdx = indices.front();

        auto& firstLight = lights[firstIdx];
        if (!firstLight || !firstLight->light)
            return;

        auto firstConfigID = firstLight->light->GetLightRuntimeData().unk138;
        auto cfgIt = LightData::configIDToJsonCfg.find(firstConfigID);
        if (cfgIt == LightData::configIDToJsonCfg.end())
            return;

        // Resolve the group's display name, disambiguating with mesh path when
        // multiple groups share the same menu name.
        std::string groupMenuName = ResolveMenuName(cfgIt->second);
        if (groupMenuName == "Unknown Light" && cfgIt->second.meshPaths.empty())
            groupMenuName = "Unknown Light";

        auto nameIt = groupData.menuNameCounts.find(groupMenuName);
        if (nameIt != groupData.menuNameCounts.end() &&
            nameIt->second > 1 &&
            !cfgIt->second.meshPaths.empty())
        {
            groupMenuName += " (" + cfgIt->second.meshPaths[0] + ")";
        }

        const bool multiEntry =
            (CountJsonEntriesInFile(cfgIt->second.configPath) > 1);

        if (multiEntry)
        {
            // Collapsible group showing each light individually
            std::string header =
                groupMenuName + " (" + std::to_string(indices.size()) + ")";

            ImGuiMCP::PushID(configPath.c_str());

            if (ImGuiMCP::TreeNode(header.c_str()))
            {
                for (int i : indices)
                {
                    auto& light = lights[i];
                    if (!light || !light->light)
                        continue;

                    auto configID = light->light->GetLightRuntimeData().unk138;
                    auto it = LightData::configIDToJsonCfg.find(configID);

                    std::string itemName = (it != LightData::configIDToJsonCfg.end())
                        ? ResolveMenuName(it->second)
                        : "Unknown Light";

                    if (it != LightData::configIDToJsonCfg.end())
                    {
                        auto countIt = groupData.menuNameCounts.find(itemName);
                        if (countIt != groupData.menuNameCounts.end() &&
                            countIt->second > 1 &&
                            !it->second.meshPaths.empty())
                        {
                            itemName += " (" + it->second.meshPaths[0] + ")";
                        }
                    }

                    ImGuiMCP::PushID(i);
                    if (ImGuiMCP::Selectable(itemName.c_str(), i == selectedIndex))
                        selectedIndex = i;
                    ImGuiMCP::PopID();
                }

                ImGuiMCP::TreePop();
            }

            ImGuiMCP::PopID();
        }
        else
        {
            // Single entry — just a flat selectable
            ImGuiMCP::PushID(firstIdx);
            if (ImGuiMCP::Selectable(groupMenuName.c_str(), firstIdx == selectedIndex))
                selectedIndex = firstIdx;
            ImGuiMCP::PopID();
        }
    }

    inline void RenderLightList(
        const std::vector<RE::NiPointer<RE::BSLight>>& lights,
        int& selectedIndex)
    {
        if (!ImGuiMCP::CollapsingHeader("Loaded Light Templates"))
            return;

        LightGroupData groupData = BuildCatagorizedLights(lights);

        // Sort categories alphabetically
        std::vector<std::string> sortedCategories;
        sortedCategories.reserve(groupData.byCategory.size());
        for (auto& [cat, _] : groupData.byCategory)
            sortedCategories.push_back(cat);

        std::sort(
            sortedCategories.begin(),
            sortedCategories.end(),
            [](const std::string& a, const std::string& b)
            {
                return compareLightNames(a.c_str(), b.c_str());
            });

        // Render categorized groups under collapsible TreeNodes
        for (const auto& category : sortedCategories)
        {
            if (ImGuiMCP::TreeNode(category.c_str()))
            {
                for (const auto& configPath : groupData.byCategory.at(category))
                    DrawCatagoryGroup(configPath, groupData, lights, selectedIndex);

                ImGuiMCP::TreePop();
            }
        }

        // Render uncategorized groups at the top level
        for (const auto& configPath : groupData.uncategorized)
            DrawCatagoryGroup(configPath, groupData, lights, selectedIndex);
    }


   
    //RenderAttachRemove FUNTIONS
    ///////////////////////////////

    inline void RefreshNearbyObjects(RE::TESObjectREFR* selected, std::string& extractedMeshName)
    {
        if (!selected) {
            logger::error("no selected ref, cannot refresh nearby objects");
            return;
        }

        logger::debug("refresh lights called with mesh name, {}", extractedMeshName);

        auto* player = RE::PlayerCharacter::GetSingleton();
        auto* tes = RE::TES::GetSingleton();

        if (!player || !tes) {
            logger::error("No Player or TES in refresh nearby objects, cant refresh");
            return;
        }

        tes->ForEachReferenceInRange(player, globals::fLODFadeOutMultObjects,
            [selected, extractedMeshName](RE::TESObjectREFR* ref)
            {
                if (!ref || ref == selected) {
                    return RE::BSContainer::ForEachResult::kContinue;
                }

                const auto base = ref->GetBaseObject();

                auto model = base ? base->As<RE::TESModel>() : nullptr;
                if (!model) {
                    //  logger::warn(" coldent get model in refresh Nearby objects");
                    return RE::BSContainer::ForEachResult::kContinue;
                }

                auto  refBMeshPath = extractMeshName(model->GetModel());

                toLower(refBMeshPath);

                if (refBMeshPath != extractedMeshName) return RE::BSContainer::ForEachResult::kContinue;

                logger::debug("Base-form match found; refreshing ref {:08X}", ref->GetFormID());

                RE::ObjectRefHandle handle{ ref };
                SKSE::GetTaskInterface()->AddTask([handle]() {
                    if (auto resolvedRef = handle.get()) {
                        resolvedRef->Disable();
                        resolvedRef->Enable(false);
                    }
                    });

                return RE::BSContainer::ForEachResult::kContinue;
            });
    }

    inline std::vector<std::string> GetAllConfigKeys()
    {
        std::vector<std::string> result;
        result.reserve(256);

        std::unordered_set<std::string> seen;

        auto collect = [&](const auto& map)
            {
                for (const auto& [key, _] : map) {
                    if (seen.insert(key).second) {
                        result.push_back(key);
                    }
                }
            };

        collect(LightData::meshPathToJsonCfg);
        collect(LightData::meshPathToJsonCfgExteriors);

        return result;
    }

    inline void RemoveFromIniExcludeRefID(RE::TESObjectREFR* ref, std::string& refIDandModName)
    {

        if (refIDandModName.empty()) {
            logger::warn("RemoveFromIniExcludeRefID: Failed to build refID string.");
            return;
        }

        if (!RemoveMenuExcludedRefFromINI("Data/SKSE/Plugins/ReLight.ini", refIDandModName)) {
            logger::info("No Ref {} Found in Ini Excludes to Remove", refIDandModName);
        }

        globals::excludedRefFormIDs.erase(ref->GetFormID());
    }


}
