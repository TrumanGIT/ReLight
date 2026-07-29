#pragma once
#include "SKSEMenuFramework.h"
#include "logger.hpp"
#include "LightData.h"
#include "Utility.h"
#include "LightManager.h"

namespace UI {

    inline auto lightbulbIcon = FontAwesome::UnicodeToUtf8(0xf0eb);

    inline auto palletIcon = FontAwesome::UnicodeToUtf8(0xf53f);

    inline auto coordinatesIcon = FontAwesome::UnicodeToUtf8(0xf601);

    inline auto editorIcon = FontAwesome::UnicodeToUtf8(0xf044);

    inline auto trashIcon = FontAwesome::UnicodeToUtf8(0xf1f8);

    inline auto plusIcon = FontAwesome::UnicodeToUtf8(0xf055);

    inline auto flagIcon = FontAwesome::UnicodeToUtf8(0xf024);

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

            //if (lightName[0] != 'R' || lightName[1] != 'L')
              //  continue;

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
                "  flickerRate       {}\n"
                "  flickerMovement       {}\n"
                "  worldPos          {}\n"
                "  unk060            {}\n"
                "  configID          {}",
                lightName,
                lightRt.fade,
                cfg.startingFade,
                lightRt.radius,
                cfg.flickerIntensity,
                cfg.flickersPerSecond,
                cfg.flickerAmplitude,
                light->light->world.translate,
                light->unk060,
                cfg.configID
            );
            float r = lightRt.diffuse.red;
            float g = lightRt.diffuse.green;
            float b = lightRt.diffuse.blue;

            logger::debug("  color (raw)       : R={:.3f}, G={:.3f}, B={:.3f}", r, g, b);
            logger::debug("  color (0-255)     : R={:.0f}, G={:.0f}, B={:.0f}", r * 255.0f, g * 255.0f, b * 255.0f);

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

        cfg.diffuseColor[0] = backupCfg.diffuseColor[0];
        cfg.diffuseColor[1] = backupCfg.diffuseColor[1];
        cfg.diffuseColor[2] = backupCfg.diffuseColor[2];

        lightData.radius = LightData::getNiPointLightRadius(backupCfg, ref->GetScale());
        lightData.fade = backupCfg.brightness;
        LightData::setNiPointLightAmbientAndDiffuse(light.get(), backupCfg);
        LightData::setNiPointLightPos(light.get(), backupCfg);

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
      //  cfg.flickerRandomness = backupCfg.flickerRandomness;
        cfg.flags = backupCfg.flags;
        cfg.menuName = backupCfg.menuName;
        cfg.menuCategory = backupCfg.menuCategory;
        cfg.externalEmittance = backupCfg.externalEmittance;
        cfg.emittanceRegion = backupCfg.emittanceRegion; 

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
                    currentLight->light->worldBound.center = light->local.translate;

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

  
    inline int getLightKey(const RE::NiPointer<RE::BSLight>& l) {
        if (!l || !l->light) return -1;
        return l->light->GetLightRuntimeData().unk138;  // runtime configID key
    }

    inline void refreshLight(
        const RE::NiPointer<RE::BSLight>& activeLight,
        std::vector<RE::NiPointer<RE::BSLight>>& refreshedLights,
        std::unordered_map<int, int>& keyToIndex,
        std::unordered_set<int>& seen,
        std::string_view prefix)
    {
        if (!activeLight || !activeLight->light) return;

        std::string_view name{ activeLight->light->name.c_str() };
        if (name.size() < prefix.size() || name.substr(0, prefix.size()) != prefix)
            return;

        int key = activeLight->light->GetLightRuntimeData().unk138;
        if (key < 0) return;

        seen.insert(key);

        auto it = LightData::configIDToJsonCfg.find(key);
        if (it == LightData::configIDToJsonCfg.end()) {
            logger::debug("light :{} (key={}) has no json cfg entry", name, key);
        }

        auto itIdx = keyToIndex.find(key);
        if (itIdx == keyToIndex.end()) {
            refreshedLights.push_back(activeLight);
            keyToIndex[key] = (int)refreshedLights.size() - 1;
        }
        else {
            refreshedLights[itIdx->second] = activeLight;
        }
    }


    inline void refreshAllLights(
        int& selectedIndex,
        std::vector<RE::NiPointer<RE::BSLight>>& lights,
        std::string_view prefix)
    {
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

        std::unordered_map<int, int> keyToIndex;
        keyToIndex.reserve(lights.size());
        for (int i = 0; i < (int)lights.size(); ++i) {
            int key = getLightKey(lights[i]);
            if (key >= 0) keyToIndex[key] = i;
        }

        std::unordered_set<int> seen;
        seen.reserve(256);

        for (auto& activeLight : rt.activeLights) {
            refreshLight(activeLight, lights, keyToIndex, seen, prefix);
        }
        for (auto& activeShadowLight : rt.activeShadowLights) {
            refreshLight(activeShadowLight, lights, keyToIndex, seen, prefix);
        }

        lights.erase(std::remove_if(lights.begin(), lights.end(),
            [&](const RE::NiPointer<RE::BSLight>& l) {
                int key = getLightKey(l);
                return key < 0 || (seen.find(key) == seen.end());
            }),
            lights.end());

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

        if (cfgIt->second.isPluginLight)
        {
            ImGuiMCP::PushID(configPath.c_str());

            for (int i : indices)
            {
                auto& light = lights[i];

                if (!light || !light->light)
                    continue;

                auto configID =
                    light->light->GetLightRuntimeData().unk138;

                auto it =
                    LightData::configIDToJsonCfg.find(configID);

                if (it == LightData::configIDToJsonCfg.end())
                    continue;

                std::string itemName =
                    ResolveMenuName(it->second);

                ImGuiMCP::PushID(i);

                if (ImGuiMCP::Selectable(
                    itemName.c_str(),
                    i == selectedIndex))
                {
                    selectedIndex = i;
                }

                ImGuiMCP::PopID();
            }

            ImGuiMCP::PopID();

            return;
        }

        const bool multiEntry =
            CountJsonEntriesInFile(cfgIt->second.configPath) > 1;

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
        int& selectedIndex,
        const char* headerLabel, bool isPluginLights)
    {
        if (!ImGuiMCP::CollapsingHeader(headerLabel))
            return;

        ImGuiMCP::PushID(headerLabel);

        LightGroupData groupData = BuildCatagorizedLights(lights);

        constexpr float maxListHeight = 450.0f;
        constexpr float categoryHeight = 24.0f;
        constexpr float itemHeight = 24.0f;

        float desiredHeight = 0.0f;
        desiredHeight += static_cast<float>(groupData.byCategory.size()) * categoryHeight;
        desiredHeight += static_cast<float>(groupData.uncategorized.size()) * itemHeight;

        constexpr float minListHeight = 150.0f;
        desiredHeight = std::clamp(desiredHeight, minListHeight, maxListHeight);

        if (ImGuiMCP::BeginChild("LightListChild", ImGuiMCP::ImVec2(0, desiredHeight), true))
        {
            std::vector<std::string> sortedCategories;
            sortedCategories.reserve(groupData.byCategory.size());
            for (auto& [cat, _] : groupData.byCategory)
                sortedCategories.push_back(cat);

            std::sort(sortedCategories.begin(), sortedCategories.end(),
                [](const std::string& a, const std::string& b)
                {
                    return compareLightNames(a.c_str(), b.c_str());
                });

            for (const auto& category : sortedCategories)
            {
                if (ImGuiMCP::TreeNode(category.c_str()))
                {
                    for (const auto& configPath : groupData.byCategory.at(category))
                    {
                        DrawCatagoryGroup(configPath, groupData, lights, selectedIndex);
                    }
                    ImGuiMCP::TreePop();
                }
            }

            for (const auto& configPath : groupData.uncategorized)
            {
                DrawCatagoryGroup(configPath, groupData, lights, selectedIndex);
            }
        }
        ImGuiMCP::EndChild();

        ImGuiMCP::PopID();
    }

    struct ActiveLightSelection {
        std::vector<RE::NiPointer<RE::BSLight>>* list = nullptr;
        int* index = nullptr;

        bool valid() const {
            return list && index && *index >= 0 && *index < (int)list->size();
        }

        RE::NiPointer<RE::BSLight> get() const {
            return valid() ? (*list)[*index] : RE::NiPointer<RE::BSLight>{};
        }
    };

    inline ActiveLightSelection ResolveActiveSelection(
        std::vector<RE::NiPointer<RE::BSLight>>& relightLights, int& relightSelectedIndex,
        std::vector<RE::NiPointer<RE::BSLight>>& pluginLights, int& pluginSelectedIndex)
    {
        if (relightSelectedIndex >= 0 && relightSelectedIndex < (int)relightLights.size())
            return { &relightLights, &relightSelectedIndex };

        if (pluginSelectedIndex >= 0 && pluginSelectedIndex < (int)pluginLights.size())
            return { &pluginLights, &pluginSelectedIndex };

        return {};
    }

  inline void RenderRelightFlags(uint32_t& flags)
    {
        static bool showFlagWindow = false;

        if (ImGuiMCP::Button((std::string("Flags ") + flagIcon).c_str())) {
            showFlagWindow = !showFlagWindow;
        }

        if (showFlagWindow) {
            ImGuiMCP::Begin(
                "Flag Window",
                &showFlagWindow,
                ImGuiMCP::ImGuiWindowFlags_::ImGuiWindowFlags_None
            );

            auto FlagCheckbox = [](const char* label, uint32_t& flags, LIGHT_FLAGS flag, const char* tooltip)
                {
                    bool checked = (flags & static_cast<uint32_t>(flag)) != 0;

                    if (ImGuiMCP::Checkbox(label, &checked)) {
                        if (checked) {
                            flags |= static_cast<uint32_t>(flag);
                        }
                        else {
                            flags &= ~static_cast<uint32_t>(flag);
                        }
                    }

                    if (ImGuiMCP::IsItemHovered()) {
                        ImGuiMCP::SetTooltip("%s", tooltip);
                    }
                };

            FlagCheckbox("Candle", flags, LIGHT_FLAGS::kCandle,
                "Important for light flicker prevention. Also allows the light to merge with any other "
                "light created from a relight json file which also has the Candle flag.");

            FlagCheckbox("Chandelier", flags, LIGHT_FLAGS::kChandelier,
                "Important for light flicker prevention.");

            FlagCheckbox("Fire", flags, LIGHT_FLAGS::kFire,
                "Important for light flicker prevention. Also allows the light to merge with any other "
                "light created from a relight json file which also has the Fire flag.");

            FlagCheckbox("Giant Campfire", flags, LIGHT_FLAGS::kGiantCampfire,
                "Allows merging with any other light created by a relight json config with the Fire flag (needed).");

            FlagCheckbox("Other", flags, LIGHT_FLAGS::kOther,
                "Allows merging with any other light created by a relight json config with the Other flag.");

            FlagCheckbox("Increased Merge Distance", flags, LIGHT_FLAGS::kIncreasedMergeDistance,
                "Grants a significantly larger merge distance to lights. Currently used for ruin candles.");

            FlagCheckbox("Increased Menu XYZ Scale", flags, LIGHT_FLAGS::kIncreasedMenuXYZScale,
                "Increases the in-game menu XYZ position slider range from 250 to 1250, for positioning "
                "lights on larger objects.");

            FlagCheckbox("No Merging", flags, LIGHT_FLAGS::kNoMerging,
                "Light sources of this type will never merge.");

            FlagCheckbox("Outdoor", flags, LIGHT_FLAGS::kOutdoor,
                "Light source is to be applied outdoors only. Lets you have interior/exterior lighting "
                "separated -- e.g. an Outdoor-flagged lantern vs a regular one with no flags.");

            FlagCheckbox("Pulse", flags, LIGHT_FLAGS::kPulse,
                "Light will pulse instead of flicker, good for flower lights ect.");

            ImGuiMCP::End();
        }
    }

  inline void RenderTESLightFlags(std::uint32_t& flags)
  {
      static bool showFlagWindow = false;

      if (ImGuiMCP::Button((std::string("Flags ") + flagIcon).c_str())) {
          showFlagWindow = !showFlagWindow;
      }

      if (showFlagWindow) {
          ImGuiMCP::Begin(
              "TES Light Flags",
              &showFlagWindow,
              ImGuiMCP::ImGuiWindowFlags_::ImGuiWindowFlags_None
          );

          auto FlagCheckbox =
              [](const char* label,
                  std::uint32_t& flags,
                  uint32_t flag,
                  const char* tooltip)
              {
                  const auto flagValue = static_cast<std::uint32_t>(flag);
                  bool checked = (flags & flagValue) != 0;

                  if (ImGuiMCP::Checkbox(label, &checked)) {
                      if (checked) {
                          flags |= flagValue;
                      }
                      else {
                          flags &= ~flagValue;
                      }
                  }

                  if (ImGuiMCP::IsItemHovered()) {
                      ImGuiMCP::SetTooltip("%s", tooltip);
                  }
              };

          FlagCheckbox(
              "Dynamic",
              flags,
              static_cast<uint32_t>(RE::TES_LIGHT_FLAGS::kDynamic),
              "Light is dynamic."
          );

          FlagCheckbox(
              "Can Carry",
              flags,
              static_cast<uint32_t>(RE::TES_LIGHT_FLAGS::kCanCarry),
              "Light can be carried."
          );

          FlagCheckbox(
              "Negative",
              flags,
              static_cast<uint32_t>(RE::TES_LIGHT_FLAGS::kNegative),
              "Light has negative lighting."
          );

          FlagCheckbox(
              "Deep Copy",
              flags,
              static_cast<uint32_t>(RE::TES_LIGHT_FLAGS::kDeepCopy),
              "Light data is deep copied."
          );

          FlagCheckbox(
              "Off By Default",
              flags,
              static_cast<uint32_t>(RE::TES_LIGHT_FLAGS::kOffByDefault),
              "Light is off by default."
          );

          ImGuiMCP::BeginDisabled(true);

          FlagCheckbox(
              "Flicker",
              flags,
              static_cast<uint32_t>(RE::TES_LIGHT_FLAGS::kFlicker),
              "Light flickers."
          );

          if (ImGuiMCP::IsItemHovered()) {
              ImGuiMCP::SetTooltip(
                  "Flicker is controlled via the flicker settings menu"
              );
          }

          FlagCheckbox(
              "Flicker Slow",
              flags,
              static_cast<uint32_t>(RE::TES_LIGHT_FLAGS::kFlickerSlow),
              "Light uses the slow flicker behavior."
          );

          if (ImGuiMCP::IsItemHovered()) {
              ImGuiMCP::SetTooltip(
                  "Flicker is controlled via the flicker settings menu"
              );
          }

          FlagCheckbox(
              "Pulse",
              flags,
              static_cast<uint32_t>(RE::TES_LIGHT_FLAGS::kPulse),
              "Light pulses instead of flickering."
          );

          if (ImGuiMCP::IsItemHovered()) {
              ImGuiMCP::SetTooltip(
                  "Flicker is controlled via the flicker settings menu"
              );
          }

          FlagCheckbox(
              "Pulse Slow",
              flags,
              static_cast<uint32_t>(RE::TES_LIGHT_FLAGS::kPulseSlow),
              "Light uses the slow pulse behavior."
          );

          if (ImGuiMCP::IsItemHovered()) {
              ImGuiMCP::SetTooltip(
                  "Flicker is controlled via the flicker settings menu"
              );
          }

          ImGuiMCP::EndDisabled();

          FlagCheckbox(
              "Spotlight",
              flags,
              static_cast<uint32_t>(RE::TES_LIGHT_FLAGS::kSpotlight),
              "Light is treated as a spotlight."
          );

          FlagCheckbox(
              "Spot Shadow",
              flags,
              static_cast<uint32_t>(RE::TES_LIGHT_FLAGS::kSpotShadow),
              "Light casts a spotlight shadow."
          );

          FlagCheckbox(
              "Hemi Shadow",
              flags,
              static_cast<uint32_t>(RE::TES_LIGHT_FLAGS::kHemiShadow),
              "Light uses hemispherical shadowing."
          );

          FlagCheckbox(
              "Omni Shadow",
              flags,
              static_cast<uint32_t>(RE::TES_LIGHT_FLAGS::kOmniShadow),
              "Light casts an omnidirectional shadow."
          );

          FlagCheckbox(
              "Portal Strict",
              flags,
              static_cast<uint32_t>(RE::TES_LIGHT_FLAGS::kPortalStrict),
              "Light uses strict portal behavior this should be on for shadow lights, i personally have it on for every light."
          );

          if(globals::islInstalled){
          // --- Extended Flags ---
          ImGuiMCP::SeparatorText("Community Shaders Flags");

          FlagCheckbox(
              "Inverse Square",
              flags,
              static_cast<uint32_t>(TES_LIGHT_FLAGS_EXT::kInverseSquare),
              "Uses inverse square falloff for attenuation for community shaders only."
          );

          FlagCheckbox(
              "Linear",
              flags,
              static_cast<uint32_t>(TES_LIGHT_FLAGS_EXT::kLinear),
              "Uses linear falloff for attenuation for community shaders only ."
          );
          }
          ImGuiMCP::End();
      }
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


  inline void BuildRegionList(std::vector<std::pair<std::string, RE::TESRegion*>>& out)
  {
      out.clear();

      auto& regions = RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESRegion>();

      for (auto* region : regions) {
          if (!region) {
              continue;
          }

          auto editorID = clib_util::editorID::get_editorID(region);
          if (editorID.empty()) {
              continue;
          }

          // filter: only FX* or Weather*
          if (!editorID.starts_with("FX") &&
              !editorID.starts_with("Weather")) {
              continue;
          }

          out.emplace_back(editorID, region);
      }

      std::sort(out.begin(), out.end(),
          [](const auto& a, const auto& b) {
              return a.first < b.first;
          });
  }

    //RenderAttachRemove FUNTIONS
    ///////////////////////////////

    inline void RefreshNearbyObjectsByBase(RE::TESObjectREFR* selected, RE::FormID targetBaseFormID)
    {
        if (!selected) {
            logger::error("no selected ref, cannot refresh nearby objects");
            return;
        }

        logger::debug("refresh lights called with base formID, {:08X}", targetBaseFormID);

        auto* player = RE::PlayerCharacter::GetSingleton();
        auto* tes = RE::TES::GetSingleton();

        if (!player || !tes) {
            logger::error("No Player or TES in refresh nearby objects, cant refresh");
            return;
        }

        tes->ForEachReferenceInRange(player, globals::fLODFadeOutMultObjects,
            [selected, targetBaseFormID](RE::TESObjectREFR* ref)
            {
                if (!ref || ref == selected) {
                    return RE::BSContainer::ForEachResult::kContinue;
                }

                const auto base = ref->GetBaseObject();
                if (!base || base->GetFormID() != targetBaseFormID) {
                    return RE::BSContainer::ForEachResult::kContinue;
                }

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

   


}
