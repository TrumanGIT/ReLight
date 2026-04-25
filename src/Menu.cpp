#include "Menu.h"
#include "ticker.h"
#include "global.h"
#include "Utility.h"
#include "lightManager.h"
#include "disableLights.h"
#include <format>

namespace logger = SKSE::log;

namespace UI {

	static RefreshTicker lightRefreshTicker(std::chrono::seconds(1));
    static buttonTicker saveButton{};
    static buttonTicker saveINIButton{};
    static buttonTicker defaultButton{};
    static vector<RE::NiPointer<RE::BSLight>> lights = {};
    static bool lightsLoaded = false;
    static bool enableLightEditor = false;
    static bool lightAlreadyInList = false;

    auto lightbulbIcon = FontAwesome::UnicodeToUtf8(0xf0eb);

    auto palletIcon = FontAwesome::UnicodeToUtf8(0xf53f); 

    auto coordinatesIcon = FontAwesome::UnicodeToUtf8(0xf601);

    auto editorIcon = FontAwesome::UnicodeToUtf8(0xf044);

    void Register() {
        if (!SKSEMenuFramework::IsInstalled()) return;

        SKSEMenuFramework::SetSection("ReLight");

        SKSEMenuFramework::AddSectionItem("Settings", UI::RenderSettings);

        SKSEMenuFramework::AddSectionItem("Light Editor", UI::RenderLightEditor);

        SKSEMenuFramework::AddSectionItem("Attach Lights", UI::RenderAttachRemove);

        SKSEMenuFramework::AddSectionItem("Light Flicker Prevention", UI::RenderLightFlickerPreventionMenu);
    }

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

    void __stdcall RenderLightFlickerPreventionMenu() {

        ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text, ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });

        FontAwesome::PushSolid();

        ImGuiMCP::Text("Light Flicker Prevention");
        ImGuiMCP::PopStyleColor();

        ImGuiMCP::SameLine();

        bool saveINIClicked = ImGuiMCP::Button("Save INI");

        ImGuiMCP::ImVec2 rectMax;
        ImGuiMCP::GetItemRectMax(&rectMax);
        ImGuiMCP::ImVec2 rectMin;
        ImGuiMCP::GetItemRectMin(&rectMin);
        ImGuiMCP::ImVec2 winPos;
        ImGuiMCP::GetWindowPos(&winPos);

        float iconX = (rectMax.x - winPos.x) + 10.0f;
        float iconY = (rectMin.y - winPos.y) + 4.0f;
        if (saveINIClicked) {
            bool ok = false;
            saveINIButton.set(buttonState::Working);
            ok = saveSettingsToIni();
            saveINIButton.set(ok ? buttonState::Success : buttonState::Fail, 2.0f);
        }
        renderDone(saveINIButton, iconX, iconY);

        ImGuiMCP::Spacing();

        if (ImGuiMCP::Checkbox("Enable Light flicker prevention", &globals::enableLightFlickerPreventionMeasures)) {
        }

        if (ImGuiMCP::IsItemHovered()) {
            ImGuiMCP::SetTooltip("Only the 7 closest lights can affect a surface");
        }
  
        ImGuiMCP::Spacing();

        if (ImGuiMCP::BeginChild("Light Merge", ImGuiMCP::ImVec2(0, 325), true,
            ImGuiMCP::ImGuiWindowFlags_NoScrollbar))
        {
            ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text, ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });

            ImGuiMCP::Text("Light Merge");
            ImGuiMCP::PopStyleColor();

            ImGuiMCP::ImVec2 avail{};
            ImGuiMCP::GetContentRegionAvail(&avail);

            ImGuiMCP::Columns(2, "Bound", false);
            ImGuiMCP::SetColumnWidth(0, avail.x * 0.5f);
            ImGuiMCP::SetColumnWidth(1, avail.x * 0.5f);

            float colWidth = avail.x * 0.25f;
            ImGuiMCP::PushItemWidth(colWidth);

            ImGuiMCP::SliderInt("Max lights to merge", &globals::lightMergeMaxLights, 0, 25);

            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("will merge no more then this amount of lights during 1 merge");
            }

            ImGuiMCP::SliderFloat("Distance to light merge", &globals::lightMergeDistance, 0, 300);

            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("Refs placed further apart will not merge");
            }

            ImGuiMCP::SliderFloat("Merge distance increased", &globals::lightMergeSeekingDistance, 0, 300);

            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("Used for configs with the IncreasedMergeDistance flag");
            }

            ImGuiMCP::SliderFloat("Merge distance shadow light", &globals::shadowLightMergeDistance, 0, 300);

            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("Don't turn this down its for fire meshes stacked on top of each other");
            }

            ImGuiMCP::SliderFloat("Z distance allowed to merge", &globals::fMaxZDiffToMerge, 0, 300);

            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("If z distance is greater, will not merge");
            }

            ImGuiMCP::SliderFloat("Z distance Increased", &globals::fMaxZDiffToMergeIncreased, 0, 300);

            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("For configs with the IncreasedMergeDistance flag");
            }

            ImGuiMCP::NextColumn(); 
            ImGuiMCP::PushItemWidth(colWidth);

            ImGuiMCP::SliderFloat("Fade Boost per Merge", &globals::lightFadePerMerge, 0.0f, 1.0f);
            if (ImGuiMCP::IsItemHovered())
                ImGuiMCP::SetTooltip("Increase in fade per additional merged light.");

            ImGuiMCP::SliderFloat("Radius Boost per Merge", &globals::lightRadiusPerMerge, 0.0f, 1.0f);
            if (ImGuiMCP::IsItemHovered())
                ImGuiMCP::SetTooltip("Increase in radius per additional merged light.");

            ImGuiMCP::SliderFloat("Max Fade Multiplier", &globals::lightFadeMax, 1.0f, 5.0f);
            if (ImGuiMCP::IsItemHovered())
                ImGuiMCP::SetTooltip("Max fade mult after merging.");

            ImGuiMCP::SliderFloat("Max Radius Multiplier", &globals::lightRadiusMax, 1.0f, 5.0f);
            if (ImGuiMCP::IsItemHovered())
                ImGuiMCP::SetTooltip("Max radius mult after merging.");
  
            ImGuiMCP::PopItemWidth(); 
        }

        ImGuiMCP::EndChild();
        if (ImGuiMCP::Button("Debug log all lights")) {
            debugLogAllLights(); 
        }

    }

    void __stdcall RenderSettings() {
        ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text, ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });

        FontAwesome::PushSolid();
        auto iconUtf8 = FontAwesome::UnicodeToUtf8(0xf0eb);

        ImGuiMCP::Text("%s ReLight Menu", iconUtf8.c_str());
        ImGuiMCP::PopStyleColor();
        ImGuiMCP::SameLine();

        bool saveINIClicked = ImGuiMCP::Button("Save INI");

        if (ImGuiMCP::IsItemHovered())
            ImGuiMCP::SetTooltip("Write current settings to ReLight.ini");

        ImGuiMCP::ImVec2 rectMax;
        ImGuiMCP::GetItemRectMax(&rectMax);
        ImGuiMCP::ImVec2 rectMin;
        ImGuiMCP::GetItemRectMin(&rectMin);
        ImGuiMCP::ImVec2 winPos;
        ImGuiMCP::GetWindowPos(&winPos);

        float iconX = (rectMax.x - winPos.x) + 10.0f;
        float iconY = (rectMin.y - winPos.y) + 4.0f;

        if (saveINIClicked) {
            bool ok = false;
            saveINIButton.set(buttonState::Working);
            ok = saveSettingsToIni();
            saveINIButton.set(ok ? buttonState::Success : buttonState::Fail, 2.0f);
        }

        renderDone(saveINIButton, iconX, iconY);
        
        ImGuiMCP::Separator();

        ImGuiMCP::Checkbox("Remove Fake Glow Orbs", &globals::removeFakeGlowOrbs);
        if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("Remove fake glow orbs used by Bethesda");

        ImGuiMCP::Checkbox("Enable Debugging Light Bulbs", &globals::enableDebugLightBulbs);
        if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("Show Creation Kit Style Light Bulbs Where Lights Were Placed");

        ImGuiMCP::Separator();

        if (ImGuiMCP::SliderInt("Logging Level", &globals::loggingLevel, 0, 3)) {
            spdlog::level::level_enum lvl;
            switch (globals::loggingLevel) {
            case 0: lvl = spdlog::level::critical; break;
            case 1: lvl = spdlog::level::err;      break;
            case 2: lvl = spdlog::level::info;     break;
            case 3: lvl = spdlog::level::debug;    break;
            default: lvl = spdlog::level::info;    break;
            }
            spdlog::set_level(lvl); 
        }

        ImGuiMCP::Separator();

        if (ImGuiMCP::CollapsingHeader("Whitelist (by plugin name)")) {
            for (auto& entry : globals::whitelist)
                ImGuiMCP::Text("%s", entry.c_str());
        }

        if (ImGuiMCP::CollapsingHeader("Priority Nodes")) {
            for (auto& entry : globals::priorityList)
                ImGuiMCP::Text("%s", entry.c_str());
        }

        if (ImGuiMCP::CollapsingHeader("Excluded Mesh Paths (Exact)")) {
            for (auto& entry : globals::meshPathExclusionList)
                ImGuiMCP::Text("%s", entry.c_str());
        }

        if (ImGuiMCP::CollapsingHeader("Excluded Mesh Paths (Partial Match)")) {
            for (auto& entry : globals::meshPathExclusionListPartialMatch)
                ImGuiMCP::Text("%s", entry.c_str());
        }
    }

    inline void makeDisplayName(std::string& name) {
        name = removePrefix(name, "RL");
        if (name.empty()) return;
        name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
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

       /*logger::info("Active light found: {}, and its key{}, NiLight ptr={}, NiLight ptr={}",
            activeLight->light->name.c_str(),
            key,
            static_cast<void*>(activeLight->light.get()), static_cast<void*>(activeLight.get()));*/ 

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

    bool compareLightNames(const char* a, const char* b) {
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

    void refreshAllLights(int& selectedIndex) {

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

        std::sort(lights.begin(), lights.end(),
            [](const RE::NiPointer<RE::BSLight>& a, const RE::NiPointer<RE::BSLight>& b)
            {
                if (!a || !a->light) return true;
                if (!b || !b->light) return false;

                const char* nameA = a->light->name.c_str();
                const char* nameB = b->light->name.c_str();

				return compareLightNames(nameA, nameB);
            });

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
    inline void RefreshNonRuntimeSettings(uint32_t configID)
    {
        //get config of light were trying to refresh
        auto it = LightData::configIDToJsonCfg.find(configID);

        if (it == LightData::configIDToJsonCfg.end()) {
            logger::warn("configID {} not found in config map cant refresh lights", configID);
            return;
        }

        LightConfig cfg = it->second;

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
                logger::warn("configID {} not found in config map cant refresh lights", configID);
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
                logger::warn("configID {} not found in config map cant refresh lights", configID);
                return;
            }

            if (it3->second.configPath != cfg.configPath || it3->second.jsonIndex != cfg.jsonIndex)
                continue;

            ssNode->RemoveLight(l->light.get()); 

            addLight(l->light);
        }

        globals::secondAfterCellFullyLoaded.store(false);
        LightData::ResetTriLightCache();
    }

    bool didRefreshThisFrame = false;

    void __stdcall RenderLightEditor() {

        static int selectedIndex = -1;

        ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text, ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });

        FontAwesome::PushSolid();
     

        ImGuiMCP::Text("%s Light Editor", editorIcon.c_str());
        ImGuiMCP::PopStyleColor();
        ImGuiMCP::SameLine();

        bool saveClicked = ImGuiMCP::Button("Save");

        if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("Save the currently selected light template's settings");

        ImGuiMCP::SameLine(0, 10.0f);

        bool defaultClicked = ImGuiMCP::Button("Default");

        if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("Restore the currently selected light template's settings to what they were at game start");

        ImGuiMCP::ImVec2 rectMax;
        ImGuiMCP::GetItemRectMax(&rectMax);
        ImGuiMCP::ImVec2 rectMin;
        ImGuiMCP::GetItemRectMin(&rectMin);
        ImGuiMCP::ImVec2 winPos;
        ImGuiMCP::GetWindowPos(&winPos);

        float iconX = (rectMax.x - winPos.x) + 10.0f;
        float iconY = (rectMin.y - winPos.y) + 4.0f;

        if (saveClicked) {

            bool ok = false;
            saveButton.set(buttonState::Working);

            if (selectedIndex >= 0 && selectedIndex < lights.size()) {
                RE::NiPointer<RE::BSLight> selectedLight = lights[selectedIndex];
                auto niLight = selectedLight->light.get();
                if (!niLight) {
                    logger::error("no ni light from bslight when saving template");
                    return;
                }

                std::string lightNameRL = niLight->name.c_str();

                if (lightNameRL.empty()) {
                    logger::debug("Nilight name empty when saving template");
                    return;
                }

                auto lightName = removePrefix(lightNameRL, "RL");

                LightConfig cfg;

                if (LightData::foundConfigForLight(niLight)) {

                    // not using runtime here i dont really know the diff 
                    // here we grab the config itself from the config to json ID map
                    auto& baseConfig = LightData::configIDToJsonCfg[niLight->unk138];

                    LightData::updateConfigFromLight(cfg, baseConfig, niLight);

                    if (!LightData::updateRuntimeConfigCaches(cfg)) {
                        logger::warn("Failed to update runtime config caches for '{}'", lightName);
                    }

                    if (!cfg.configPath.empty()) {
                        saveConfiguration(cfg);
                        ok = true;
                    }
                    else {
                        logger::warn("Config for '{}' has no configPath, cannot save", lightName);
                        ok = false;
                    }
                }
                else {
                    logger::warn("No config found for light '{}'", lightName);
                    ok = false;
                }
            }
            else {
                logger::warn("Save clicked but no light selected");
                ok = false;
            }

            saveButton.set(ok ? buttonState::Success : buttonState::Fail, 2.0f);
        }

        if (defaultClicked) {

            bool ok = false;
            defaultButton.set(buttonState::Working);

            if (selectedIndex >= 0 && selectedIndex < lights.size()) {
                auto selectedLight = lights[selectedIndex];
                restoreLightToDefaults(selectedLight->light);
                logger::info("Restored defaults for '{}'", selectedLight->light->name.c_str());
                ok = true;
            }
            else {
                logger::warn("Default clicked but no light selected");
                ok = false;
            }

            defaultButton.set(ok ? buttonState::Success : buttonState::Fail, 2.0f);
        }

        renderDone(saveButton, iconX, iconY);
        renderDone(defaultButton, iconX, iconY);

        ImGuiMCP::Separator();

        if (lightRefreshTicker.shouldTick()) {
            refreshAllLights(selectedIndex);
            didRefreshThisFrame = !didRefreshThisFrame;
        }

        if (ImGuiMCP::CollapsingHeader("Loaded Light Templates")) {

            // Count duplicates first
            std::unordered_map<std::string, int> nameCounts;
            std::unordered_map<std::string, int> nameIndex;
            for (int i = 0; i < lights.size(); i++) {

                auto& light = lights[i];
                if (!light) continue;

                auto lightName = removePrefix(light->light->name.c_str(), "RL");

                std::string menuName;

                auto it = LightData::configIDToJsonCfg.find(light->light->GetLightRuntimeData().unk138);
                if (it != LightData::configIDToJsonCfg.end()) {

                    menuName = it->second.menuName;

                    if (menuName.empty()) {

                        logger::debug("menu name empty for config {}", it->second.configID);
                        menuName = std::string(light->light->name.c_str());
                    }
                }

                makeDisplayName(menuName);

                nameCounts[menuName]++;
            }

            // Then render with duplicates count in name
            for (int i = 0; i < lights.size(); i++) {

                auto& light = lights[i];
                if (!light) continue;

                bool selected = (i == selectedIndex);

                auto lightName = removePrefix(light->light->name.c_str(), "RL");

                std::string menuName;

                auto it = LightData::configIDToJsonCfg.find(light->light->GetLightRuntimeData().unk138);
                if (it != LightData::configIDToJsonCfg.end()) {
                    menuName = it->second.menuName;

                    if (menuName.empty()) {

                        logger::debug("menu name empty for config {}", it->second.configID);
                        menuName = std::string(light->light->name.c_str());
                    }
                }

                makeDisplayName(menuName);

                if (nameCounts[menuName] > 1) {
                    const int index = ++nameIndex[menuName];
                    menuName = std::format("{} {} ({})", menuName, index, lightName);
                }

                // If 2 lights in the menu has same menu name they cannot be selected, 
                // ImGuiMCP::PushID(i) fixes this by giving each selectable a unique ID
                ImGuiMCP::PushID(i);
                if (ImGuiMCP::Selectable(menuName.c_str(), &selected)) {
                    selectedIndex = i;
                }
                ImGuiMCP::PopID();
            }
        }

            if (selectedIndex >= 0 && selectedIndex < lights.size()) {
                RE::NiPointer<RE::BSLight> selectedLight = lights[selectedIndex];
                auto& lightData = selectedLight->light->GetLightRuntimeData();
                auto it = LightData::configIDToJsonCfg.find(lightData.unk138);
                if (it == LightData::configIDToJsonCfg.end())
                    return;

                auto& config = it->second;

                Overlay* selectedIslRt;

                if (globals::islInstalled) {
                    selectedIslRt = Overlay::Get(selectedLight->light.get());

                    if (!selectedIslRt)
                        return;
                }

                ImGuiMCP::PushID(selectedLight->light.get());

                ImGuiMCP::Dummy(ImGuiMCP::ImVec2(0.0f, 10.0f));
                ImGuiMCP::PushItemWidth(150.0f);

                ImGuiMCP::Columns(2, nullptr, false);


                if (ImGuiMCP::BeginChild("BrightnessBox", ImGuiMCP::ImVec2(0, 200), true,
                    ImGuiMCP::ImGuiWindowFlags_NoScrollbar))
                {
                    ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text,
                        ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });
                    FontAwesome::PushSolid();

                    ImGuiMCP::Text("%s Illuminance", lightbulbIcon.c_str());
                    ImGuiMCP::PopStyleColor();
                    ImGuiMCP::Separator();

                    if (ImGuiMCP::SliderFloat("Brightness", &config.startingFade, 0.0f, 10.0f, "%.1f")) {
                        auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                        if (ssNode) {
                            auto& rt = ssNode->GetRuntimeData();
                            for (auto& l : rt.activeLights) {
                                if (!l) continue;
                                auto& rtData = l->light->GetLightRuntimeData();
                                if (rtData.unk138 == lightData.unk138)
                                    rtData.fade = config.startingFade;
                            }
                            for (auto& l : rt.activeShadowLights) {
                                if (!l) continue;
                                auto& rtData = l->light->GetLightRuntimeData();
                                if (rtData.unk138 == lightData.unk138)
                                    rtData.fade = config.startingFade;
                            }
                        }
                    }

                    if (!globals::islInstalled) {
                        if (ImGuiMCP::SliderFloat("Radius", &lightData.radius.x, 1.0f, 500.0f, "%.2f")) {
                            auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                            if (ssNode) {
                                auto& rt = ssNode->GetRuntimeData();
                                for (auto& l : rt.activeLights) {
                                    if (!l) continue;
                                    auto& rtData = l->light->GetLightRuntimeData();
                                    if (rtData.unk138 == lightData.unk138)
                                        rtData.radius = lightData.radius;
                                }
                                for (auto& l : rt.activeShadowLights) {
                                    if (!l) continue;
                                    auto& rtData = l->light->GetLightRuntimeData();
                                    if (rtData.unk138 == lightData.unk138)
                                        rtData.radius = lightData.radius;
                                }
                            }
                        }
                    }
                    else if (selectedIslRt) {
                        if (ImGuiMCP::SliderFloat("Cutoff (ISL)", &selectedIslRt->cutoffOverride, 0.01f, 0.99f, "%.2f")) {
                            auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                            if (ssNode) {
                                auto& rt = ssNode->GetRuntimeData();
                                for (auto& l : rt.activeLights) {
                                    if (!l) continue;
                                    if (l->light->GetLightRuntimeData().unk138 != lightData.unk138) continue;
                                    if (auto* isl = Overlay::Get(l->light.get())) {
                                        isl->cutoffOverride = selectedIslRt->cutoffOverride;
                                    }
                                }
                                for (auto& l : rt.activeShadowLights) {
                                    if (!l) continue;
                                    if (l->light->GetLightRuntimeData().unk138 != lightData.unk138) continue;
                                    if (auto* isl = Overlay::Get(l->light.get())) {
                                        isl->cutoffOverride = selectedIslRt->cutoffOverride;
                                    }
                                }
                            }
                        }

                        if (ImGuiMCP::SliderFloat("Size (ISL)", &selectedIslRt->size, 0.0f, 10.0f, "%.2f")) {
                            auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                            if (ssNode) {
                                auto& rt = ssNode->GetRuntimeData();
                                for (auto& l : rt.activeLights) {
                                    if (!l) continue;
                                    if (l->light->GetLightRuntimeData().unk138 != lightData.unk138) continue;
                                    if (auto* isl = Overlay::Get(l->light.get())) {
                                        isl->size = selectedIslRt->size;
                                    }
                                }
                                for (auto& l : rt.activeShadowLights) {
                                    if (!l) continue;
                                    if (l->light->GetLightRuntimeData().unk138 != lightData.unk138) continue;
                                    if (auto* isl = Overlay::Get(l->light.get())) {
                                        isl->size = selectedIslRt->size;
                                    }
                                }
                            }
                        }
                    }
                }
                ImGuiMCP::EndChild();
                ImGuiMCP::NextColumn();

                bool isTorch = (selectedLight->light->name == "RLtorch");

                if (ImGuiMCP::BeginChild(
                    "FlickerBox",
                    ImGuiMCP::ImVec2(0, 200),
                    true,
                    ImGuiMCP::ImGuiWindowFlags_NoScrollbar))
                {
                    // ----- Flicker Header -----

                    // ICON (dynamic)
                    if (didRefreshThisFrame && config.flickersPerSecond != 0.0f) {
                        FontAwesome::PushSolid();
                        ImGuiMCP::PushStyleColor(
                            ImGuiMCP::ImGuiCol_Text,
                            ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });
                    }
                    else {
                        FontAwesome::PushRegular();
                        ImGuiMCP::PushStyleColor(
                            ImGuiMCP::ImGuiCol_Text,
                            ImGuiMCP::ImVec4{ 0.35f, 0.35f, 0.35f, 1.0f });
                    }

                    ImGuiMCP::Text("%s", lightbulbIcon.c_str());

                    ImGuiMCP::PopStyleColor();
                    FontAwesome::Pop();

                    // TEXT (static)
                    ImGuiMCP::SameLine();
                    ImGuiMCP::PushStyleColor(
                        ImGuiMCP::ImGuiCol_Text,
                        ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });
                    ImGuiMCP::Text("Flicker");
                    ImGuiMCP::PopStyleColor();

                    ImGuiMCP::Separator();

                    // ----- Controls -----
                    ImGuiMCP::BeginDisabled(isTorch);
                    ImGuiMCP::SliderFloat(
                        "Flicker Intensity",
                        &config.flickerIntensity,
                        0.0f, 1.0f, "%.2f");

                    ImGuiMCP::SliderFloat(
                        "Flickers / Second",
                        &config.flickersPerSecond,
                        0.0f, 5.0f, "%.2f");

                    ImGuiMCP::SliderFloat(
                        "Movement",
                        &config.flickerAmplitude,
                        0.0f, 5.0f, "%.2f");
                    ImGuiMCP::EndDisabled();
                }
                ImGuiMCP::EndChild();


                ImGuiMCP::Columns(1);
                ImGuiMCP::Dummy(ImGuiMCP::ImVec2(0, 5));

                ImGuiMCP::Columns(2, nullptr, false);

                float sliderRange = (config.flags & static_cast<uint32_t>(LIGHT_FLAGS::kIncreasedMenuXYZScale))
                    ? 1250.0f
                    : 250.0f;

                if (ImGuiMCP::BeginChild(
                    "PositionBox",
                    ImGuiMCP::ImVec2(0, 100),
                    true,
                    ImGuiMCP::ImGuiWindowFlags_NoScrollbar))
                {
                    ImGuiMCP::PushStyleColor(
                        ImGuiMCP::ImGuiCol_Text,
                        ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });
                    ImGuiMCP::Text("%s Translation", coordinatesIcon.c_str());
                    ImGuiMCP::PopStyleColor();

                    ImGuiMCP::Separator();

                    ImGuiMCP::BeginDisabled(isTorch);

                    if (ImGuiMCP::SliderFloat3(
                        "Position",
                        &config.position[0],
                        -sliderRange, sliderRange, "%.3f"))
                    {
                        if (!isTorch) {
                            auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                            if (ssNode) {
                                auto& rt = ssNode->GetRuntimeData();
                                for (auto& l : rt.activeLights) {
                                    if (!l) continue;
                                    if (l->light->GetLightRuntimeData().unk138 != lightData.unk138) continue;
                                    l->light->local.translate.x = config.position[0];
                                    l->light->local.translate.y = config.position[1];
                                    l->light->local.translate.z = config.position[2];
                                    if (auto* parent = l->light->parent) {
                                        RE::NiUpdateData updateData{};
                                        updateData.time = 0.0f;
                                        updateData.flags = RE::NiUpdateData::Flag::kDirty;
                                        parent->UpdateTransformAndBounds(updateData);
                                    }
                                }
                                for (auto& l : rt.activeShadowLights) {
                                    if (!l) continue;
                                    if (l->light->GetLightRuntimeData().unk138 != lightData.unk138) continue;
                                    l->light->local.translate.x = config.position[0];
                                    l->light->local.translate.y = config.position[1];
                                    l->light->local.translate.z = config.position[2];
                                    if (auto* parent = l->light->parent) {
                                        RE::NiUpdateData updateData{};
                                        updateData.time = 0.0f;
                                        updateData.flags = RE::NiUpdateData::Flag::kDirty;
                                        parent->UpdateTransformAndBounds(updateData);
                                    }
                                }
                            }
                        }
                    }
                    ImGuiMCP::EndDisabled();
                }
                ImGuiMCP::EndChild();

                ImGuiMCP::NextColumn();


                if (ImGuiMCP::BeginChild("ColorBox", ImGuiMCP::ImVec2(0, 100), true,
                    ImGuiMCP::ImGuiWindowFlags_NoScrollbar))
                {
                    ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text,
                        ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });

                    ImGuiMCP::Text("%s Color (RGB)", palletIcon.c_str());
                    ImGuiMCP::PopStyleColor();
                    ImGuiMCP::Separator();

                    if (ImGuiMCP::SliderFloat3("RGB", &lightData.diffuse.red, 0.0f, 1.0f, "%.3f")) {
                        auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                        if (ssNode) {
                            auto& rt = ssNode->GetRuntimeData();
                            for (auto& l : rt.activeLights) {
                                if (!l) continue;
                                if (l->light->GetLightRuntimeData().unk138 == lightData.unk138)
                                    l->light->GetLightRuntimeData().diffuse = lightData.diffuse;
                            }
                            for (auto& l : rt.activeShadowLights) {
                                if (!l) continue;
                                if (l->light->GetLightRuntimeData().unk138 == lightData.unk138)
                                    l->light->GetLightRuntimeData().diffuse = lightData.diffuse;
                            }
                        }
                    }
                }
                ImGuiMCP::EndChild();

                ImGuiMCP::Columns(1);

                ImGuiMCP::Spacing();

                ImGuiMCP::Spacing();

                if (ImGuiMCP::BeginChild("NonRuntimeBox", ImGuiMCP::ImVec2(0, 200), true,
                    ImGuiMCP::ImGuiWindowFlags_NoScrollbar))
                {
                    ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text,
                        ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });
                    ImGuiMCP::Text("Non-Runtime Light Settings");
                    ImGuiMCP::PopStyleColor();

                    ImGuiMCP::SameLine();

                    ImGuiMCP::BeginDisabled(isTorch);

                    if (ImGuiMCP::Button("Refresh Lights")) {
                        RefreshNonRuntimeSettings(selectedLight->light->unk138);
                    }

                    if (ImGuiMCP::IsItemHovered()) {
                        ImGuiMCP::SetTooltip("Changes to these settings require refreshing lights to take effect.");
                    }

                    ImGuiMCP::Separator();

                    ImGuiMCP::ImVec2 avail{};
                    ImGuiMCP::GetContentRegionAvail(&avail);
                    float halfWidth = avail.x * 0.3f;

                    ImGuiMCP::Columns(2, "NonRuntimeColumns", false);

                    ImGuiMCP::Spacing();
                    ImGuiMCP::PushItemWidth(halfWidth);

                    ImGuiMCP::SliderFloat("Fall Off", &config.falloff, 0.0f, 5.0f, "%.1f");
                    ImGuiMCP::SliderFloat("Depth Bias", &config.depthBias, 0.0f, 30.0f, "%.2f");
                    ImGuiMCP::SliderFloat("FOV", &config.fov, 0.0f, 90.0f, "%.2f");
                    ImGuiMCP::NextColumn();
 
                    ImGuiMCP::SliderFloat("Near Distance", &config.nearDistance, 0.0f, 5.0f, "%.2f");
                    ImGuiMCP::Checkbox("Is Shadow Light", &config.shadowLight);

                    ImGuiMCP::EndDisabled();

                    ImGuiMCP::EndChild();
                }

                ImGuiMCP::PopID();
            }

    }


    enum class AttachLightStep
    {
        SelectTarget,
        AlreadyHasLight,
        ChooseTemplateType,
        ChooseTemplate,
        ChooseScope,
        Done,
        LightRemoved
    };

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

    void __stdcall RenderAttachRemove()
    {

        auto centerNextItem = [&](float estimatedWidth) {
            float startX = ImGuiMCP::GetCursorPosX();
            ImGuiMCP::ImVec2 avail{};
            ImGuiMCP::GetContentRegionAvail(&avail);
            ImGuiMCP::SetCursorPosX(startX + (avail.x - estimatedWidth) * 0.5f);
         };


        static AttachLightStep step = AttachLightStep::SelectTarget;
        static bool createNewTemplate = false;
        static bool multiLight = false;
        static bool refLight = false;
        bool attachedDebugMarker = false;

        static RE::FormID formID = 0x0;
        static RE::FormID baseFormID = 0x0;
        static std::string meshPath{};
        static std::string jsonFilePath{};
        static std::string menuName{};
        static std::string matched{};
        static RE::NiLight* niLight = nullptr;
        static RE::FormID lastSelected = 0;
        static RE::FormID previewRef = 0;
        static int previewSelectedIndex = -1;
        static std::vector<std::pair<std::string, LightConfig>> configDisplay;
        static std::unordered_set<std::string> seenMenuNames;
        static int selectedIndex = -1;
        static std::vector<LightConfig> selectedCfgs;
        static std::size_t entryCount = 0;

        static RE::TESObject* baseObject = nullptr;
        static RE::TESModel* model = nullptr;

        static LightConfig newCfg;

        auto resetState = [&]() {
            createNewTemplate = false;
            multiLight = false;
            refLight = false;
           attachedDebugMarker = false;

            meshPath.clear();
            jsonFilePath.clear();
            menuName.clear();

            niLight = nullptr;
            baseObject = nullptr;
            

            previewRef = 0;
            previewSelectedIndex = -1;

            configDisplay.clear();
            seenMenuNames.clear();
            selectedIndex = -1;
            selectedCfgs.clear();
            matched.clear();

            entryCount = 0;
            formID = 0x0;
            baseFormID = 0x0;
            newCfg = LightConfig{};
            step = AttachLightStep::SelectTarget;
        };


        auto selected = RE::Console::GetSelectedRef().get();

        if (!selected) {
            resetState();
            centerNextItem(330.0f);
            ImGuiMCP::Text("Click on an object in the console to continue.");
            return;
        }

        if (selected->GetFormID() != lastSelected) {
        
            resetState();

            baseObject = selected->GetBaseObject();
            if (!baseObject) {
                return;
            }

            baseFormID = baseObject->GetFormID();

            model = baseObject->As<RE::TESModel>();
            if (!model) {
                return;
            }

            meshPath = extractMeshName(model->GetModel());
            toLower(meshPath);

            lastSelected = selected->GetFormID();
        }

        if (!baseObject || !model) {

            baseObject = selected->GetBaseObject();
            if (!baseObject) {
                return;
            }

            baseFormID = baseObject->GetFormID();

            model = baseObject->As<RE::TESModel>();
            if (!model) {
                return;
            }

            meshPath = extractMeshName(model->GetModel());
            toLower(meshPath);

            lastSelected = selected->GetFormID();
            return;
        }

        switch (step)
        {
        case AttachLightStep::SelectTarget:
        {
            step = HasRelightLight(selected) ?
                AttachLightStep::AlreadyHasLight :
                AttachLightStep::ChooseTemplateType;
            break;
        }

        case AttachLightStep::AlreadyHasLight:
        {
            centerNextItem(300.0f);
            ImGuiMCP::Text("Object Selected in the console already has a ReLight light.");
      
            ImGuiMCP::Spacing(); 

            centerNextItem(500.0f);

            if (ImGuiMCP::Button("Add another Light")) {

                multiLight = true;

                auto multiLightCfg = FindRefIDConfigForAttachAnother(selected);

                if (!multiLightCfg.configPath.empty()) {
                    logger::info("Add another light: found existing ref ID config {:08X}", selected->GetFormID());

                    entryCount = CountJsonEntriesInFile(multiLightCfg.configPath);
                    jsonFilePath = multiLightCfg.configPath;
                    menuName = multiLightCfg.menuName;

                    auto root = selected->Get3D();

                    if (!root) break;

                    auto rootAsNode = root->AsNode();

                    if (!rootAsNode) break;

                    niLight = LightManager::AttachLight(multiLightCfg, rootAsNode, selected, meshPath, selected->GetFormID(), attachedDebugMarker);

                    newCfg = multiLightCfg;
                    newCfg.configID = globals::nextID++;

                 
                    refLight = true;
                    formID = selected->GetFormID();

                    SKSE::GetTaskInterface()->AddTask([]() {
                        LightData::ResetTriLightCache();
                        });
                }
                else {
                  
                    refLight = false;

                     matched = std::string(findPriorityMatch(meshPath));
                    if (matched.empty()) {
                        logger::warn("attach another light: no priority match for mesh '{}'", meshPath);
                        break;
                    }

                    logger::debug("attach another light matched = {}", matched);

                    selectedCfgs = findConfigsForMeshPath(matched, globals::currentCellIsInterior);

                    if (selectedCfgs.empty()) {
                        logger::warn("during attach another light, cfgs is empty for ref {:08X}, with name {} ", selected->GetFormID(), meshPath);
                        break;
                    }

                    menuName = selectedCfgs[0].menuName;
                    jsonFilePath = selectedCfgs[0].configPath;        
                    entryCount = CountJsonEntriesInFile(selectedCfgs[0].configPath);

                    std::string finalMenuName = menuName + " " + std::to_string(entryCount + 1);

                    newCfg = selectedCfgs[0];

                    newCfg.menuName = finalMenuName; 
                    newCfg.configID = globals::nextID++;

                    auto root = selected->Get3D(); 

                    if (!root) break; 

                    auto rootAsNode = root->AsNode(); 
                    
                    if (!rootAsNode) break; 

                    niLight = LightManager::AttachLight(newCfg, rootAsNode, selected, meshPath, selected->GetFormID(), attachedDebugMarker);
            
                    LightData::configIDToJsonCfg[newCfg.configID] = newCfg;

                    SKSE::GetTaskInterface()->AddTask([]() {
                        LightData::ResetTriLightCache();
                        });
                }

                step = AttachLightStep::Done;
                break;
            }

            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("You can edit new light in Light Editor as Torch 1, Torch 2 ect.");
            }

            ImGuiMCP::SameLine();

            if (ImGuiMCP::Button("Add To Light Exclusion List")) {

                std::string refIDandModName = BuildRefIDAndModName(selected);

                if (!AppendMenuExcludedRefToINI("Data/SKSE/Plugins/ReLight.ini", refIDandModName)) {
                    logger::error("Failed to append excluded ref {}", refIDandModName);
                }

                RE::ObjectRefHandle handle = selected->GetHandle();

                SKSE::GetTaskInterface()->AddTask([handle]() {
                    int lightsRemoved = 0;

                    if (auto ref = handle.get()) {
                        auto a_root = ref->Get3D();
                        if (!a_root) {
                            return;
                        }

                        auto node = a_root->AsNode();
                        if (!node) {
                            return;
                        }

                        std::vector<RE::NiAVObject*> childrenToDetach;
                        std::vector<RE::NiLight*> niLights;

                        for (const auto& childNode : node->GetChildren()) {
                            if (!childNode) {
                                continue;
                            }

                            auto name = std::string_view(childNode->name.c_str());

                            // Relight point lights have RL prefix
                            if (name.size() < 2 || name[0] != 'R' || name[1] != 'L') {
                                continue;
                            }

                            auto* light = netimmerse_cast<RE::NiLight*>(childNode.get());
                            if (!light) {
                                continue;
                            }

                            // collect ni point lights in the ref
                            childrenToDetach.push_back(childNode.get());
                            niLights.push_back(light);
                        }

                        auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                        if (!ssNode) {
                            logger::warn("ShadowSceneNode[0] is null!");
                            return;
                        }

                        std::vector<RE::NiPointer<RE::BSLight>> bsLightsToRemove;

                        // try to find its matching bs light and remove
                        for (const auto& bsLight : ssNode->activeLights) {
                            if (!bsLight || !bsLight->light) {
                                continue;
                            }

                            for (auto* light : niLights) {
                                if (bsLight->light.get() == light) {
                                    bsLightsToRemove.push_back(bsLight);
                                    break;
                                }
                            }
                        }

                        for (const auto& bsLight : ssNode->activeShadowLights) {
                            if (!bsLight || !bsLight->light) {
                                continue;
                            }

                            for (auto* light : niLights) {
                                if (bsLight->light.get() == light) {
                                    bsLightsToRemove.push_back(bsLight);
                                    break;
                                }
                            }
                        }

                        for (const auto& bsLight : bsLightsToRemove) {
                            if (!bsLight || !bsLight->light) {
                                continue;
                            }

                            logger::debug(
                                "BSLight with name {} for ref {:08X} has been removed from ShadowSceneNode",
                                bsLight->light->name,
                                ref->GetFormID());

                            ssNode->RemoveLight(bsLight);
                        }

                        // finally remove nilight from mesh geometry aswell 
                        for (auto* child : childrenToDetach) {
                            if (!child) {
                                continue;
                            }

                            node->DetachChild(child);
                            ++lightsRemoved;
                        }

                        logger::info("Removed {} lights for ref {:08X}", lightsRemoved, ref->GetFormID());
                    }
                    });

                step = AttachLightStep::LightRemoved;
                break;
            }
            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip(
                    "Adds to exclude by refID section in RELight.ini file, preventing object from getting a Relight\n"
                    "TIP: Can also use to change a automated light into a seperate light you can edit by itself in the light editor.\n"
                    "Just push this button, then when attaching a new light select 'this object only'"
                );
            }

            break;
        }

        case AttachLightStep::ChooseTemplateType:
        {
            centerNextItem(430.0f);       
            ImGuiMCP::Text("      Attaching light to object selected in console.\nCreate new Light Template or add to existing template?");


            ImGuiMCP::Spacing();

            centerNextItem(630.0f);

            if (ImGuiMCP::Button("Reuse an existing template")) {
                createNewTemplate = false;
                step = AttachLightStep::ChooseTemplate;
            }

            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("Reusing templates keeps your config folder uncluttered.");
            }

            ImGuiMCP::SameLine();

            if (ImGuiMCP::Button("Create a new template")) {
                createNewTemplate = true;
                newCfg = LightConfig{};
                step = AttachLightStep::ChooseScope;
            }

            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("Better if you want to control this light seperatly.");
            }
            ImGuiMCP::SameLine();

            if (ImGuiMCP::Button("Add To Light Exclusion List")) {

                std::string refIDandModName = BuildRefIDAndModName(selected);

                if (!AppendMenuExcludedRefToINI("Data/SKSE/Plugins/ReLight.ini", refIDandModName)) {
                    logger::error("Failed to append excluded ref {}", refIDandModName);
                }

                RE::ObjectRefHandle handle = selected->GetHandle();

                SKSE::GetTaskInterface()->AddTask([handle]() {
                    int lightsRemoved = 0;

                    if (auto ref = handle.get()) {
                        auto a_root = ref->Get3D();
                        if (!a_root) {
                            return;
                        }

                        auto node = a_root->AsNode();
                        if (!node) {
                            return;
                        }

                        std::vector<RE::NiAVObject*> childrenToDetach;
                        std::vector<RE::NiLight*> niLights;

                        for (const auto& childNode : node->GetChildren()) {
                            if (!childNode) {
                                continue;
                            }

                            auto name = std::string_view(childNode->name.c_str());

                            // Relight point lights have RL prefix
                            if (name.size() < 2 || name[0] != 'R' || name[1] != 'L') {
                                continue;
                            }

                            auto* light = netimmerse_cast<RE::NiLight*>(childNode.get());
                            if (!light) {
                                continue;
                            }

                            // collect ni point lights in the ref
                            childrenToDetach.push_back(childNode.get());
                            niLights.push_back(light);
                        }

                        auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                        if (!ssNode) {
                            logger::warn("ShadowSceneNode[0] is null!");
                            return;
                        }

                        std::vector<RE::NiPointer<RE::BSLight>> bsLightsToRemove;

                        // try to find its matching bs light and remove
                        for (const auto& bsLight : ssNode->activeLights) {
                            if (!bsLight || !bsLight->light) {
                                continue;
                            }

                            for (auto* light : niLights) {
                                if (bsLight->light.get() == light) {
                                    bsLightsToRemove.push_back(bsLight);
                                    break;
                                }
                            }
                        }

                        for (const auto& bsLight : ssNode->activeShadowLights) {
                            if (!bsLight || !bsLight->light) {
                                continue;
                            }

                            for (auto* light : niLights) {
                                if (bsLight->light.get() == light) {
                                    bsLightsToRemove.push_back(bsLight);
                                    break;
                                }
                            }
                        }

                        for (const auto& bsLight : bsLightsToRemove) {
                            if (!bsLight || !bsLight->light) {
                                continue;
                            }

                            logger::debug(
                                "BSLight with name {} for ref {:08X} has been removed from ShadowSceneNode",
                                bsLight->light->name,
                                ref->GetFormID());

                            ssNode->RemoveLight(bsLight);
                        }

                        // finally remove nilight from mesh geometry aswell 
                        for (auto* child : childrenToDetach) {
                            if (!child) {
                                continue;
                            }

                            node->DetachChild(child);
                            ++lightsRemoved;
                        }

                        logger::info("Removed {} lights for ref {:08X}", lightsRemoved, ref->GetFormID());
                    }
                    });

                step = AttachLightStep::LightRemoved;
                break;
            }
            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip(
                    "Adds to exclude by refID section in RELight.ini file, preventing object from getting a Relight\n"
                    "TIP: Can also use to change a automated light into a seperate light you can edit by itself in the light editor.\n"
                    "Just push this button, then when attaching a new light select 'this object only'"
                );
            }

            break;
        }

        case AttachLightStep::ChooseTemplate:
        {
            centerNextItem(120.0f);
            ImGuiMCP::Text("Select a template.");

            ImGuiMCP::Spacing();

            if (configDisplay.empty()) {
                configDisplay.clear();
                seenMenuNames.clear();

                auto tryAdd = [&](const std::string& key, const std::vector<LightConfig>& cfgVec) {
                    if (cfgVec.empty()) {
                        return;
                    }

                    const auto& cfg = cfgVec[0];

                    std::string name = cfg.menuName.empty() ? key : cfg.menuName;
                    std::string nameLower = toLowerImmut(name);

                    if (seenMenuNames.insert(nameLower).second) {
                        configDisplay.emplace_back(key, cfg);
                    }
                    };

                for (auto& [key, cfgVec] : LightData::meshPathToJsonCfg) {
                    tryAdd(key, cfgVec);
                }

                for (auto& [key, cfgVec] : LightData::meshPathToJsonCfgExteriors) {
                    tryAdd(key, cfgVec);
                }
            }

            for (int i = 0; i < static_cast<int>(configDisplay.size()); i++) {
                const auto& [key, cfg] = configDisplay[i];

                if (ImGuiMCP::Selectable(cfg.menuName.c_str(), selectedIndex == i)) {
                    selectedIndex = i;
                }
            }

            if (selectedIndex == -1) {
                centerNextItem(60.0f);
                if (ImGuiMCP::Button("Cancel")) {
                    resetState();
                }
                break;
            }

            const std::string& selectedKey = configDisplay[selectedIndex].first;
            selectedCfgs.clear();

            if (auto itCfg = LightData::meshPathToJsonCfg.find(selectedKey);
                itCfg != LightData::meshPathToJsonCfg.end()) {
                selectedCfgs = itCfg->second;
            }
            else if (auto itExt = LightData::meshPathToJsonCfgExteriors.find(selectedKey);
                itExt != LightData::meshPathToJsonCfgExteriors.end()) {
                selectedCfgs = itExt->second;
            }

            if (selectedCfgs.empty()) {
                centerNextItem(220.0f);
                ImGuiMCP::Text("Selected config was empty or not found.");
                step = AttachLightStep::ChooseTemplateType;
                selectedIndex = -1;
                break;
            }

            centerNextItem(170.0f);

            if (ImGuiMCP::Button("Confirm")) {
                step = AttachLightStep::ChooseScope;
            }

            ImGuiMCP::SameLine();

            if (ImGuiMCP::Button("Cancel")) {
                resetState();
            }

            break;
        }

        case AttachLightStep::ChooseScope:
        {
            centerNextItem(260.0f);
            ImGuiMCP::Text("This Object Only, or all objects like it?");

            ImGuiMCP::Spacing();

            centerNextItem(300.0f);

            if (ImGuiMCP::Button("This object only")) {

                refLight = true;

                if (createNewTemplate) {

                    newCfg.refFormIDAndModName = BuildRefIDAndModName(selected);
                    newCfg.menuName = newCfg.refFormIDAndModName;
                    newCfg.configPath = BuildConfigPath(newCfg.refFormIDAndModName);
                    newCfg.jsonIndex = 0;
                    newCfg.configID = globals::nextID++;
                    newCfg.diffuseColor = { 255, 162, 61 };
                    newCfg.startingFade = 3.0f;
                    auto root = selected->Get3D();

                    if (!root) break;

                    auto rootAsNode = root->AsNode();

                    if (!rootAsNode) break;

                    niLight = LightManager::AttachLight(newCfg, rootAsNode, selected, meshPath, selected->GetFormID(), attachedDebugMarker);
                    LightData::configIDToJsonCfg[newCfg.configID] = newCfg;

                    SKSE::GetTaskInterface()->AddTask([]() {
                        LightData::ResetTriLightCache();
                        });


                    RemoveFromIniExcludeRefID(selected, newCfg.refFormIDAndModName);
                }
                else {
                    auto a_root = selected->Get3D();
                    if (!a_root) {
                        logger::warn("Could not load this object's 3D cannnot attach light.");
                        break;
                    }

                    auto attachNode = a_root->AsNode();
                    if (!attachNode) {
                        logger::warn("Could not load this object's node cannnot attach light.");
                        break;
                    }

                    for (const auto& cfg : selectedCfgs) {

                        niLight = LightManager::AttachLight(cfg, attachNode, selected, meshPath, selected->GetFormID(), attachedDebugMarker);
                    }

                    UpdateRefRootTransforms(selected);

                    SKSE::GetTaskInterface()->AddTask([]() {
                        LightData::ResetTriLightCache();
                        });

                    std::string refIDandModName = BuildRefIDAndModName(selected);

                    RemoveFromIniExcludeRefID(selected, refIDandModName);

                    refIDandModName.clear();

                
                }
                step = AttachLightStep::Done;
                break;
            }

                ImGuiMCP::SameLine();

                if (ImGuiMCP::Button("All like this")) {
                    refLight = false;

                    if (createNewTemplate) {

                        newCfg.meshPath = meshPath;
                        newCfg.menuName = meshPath;
                        newCfg.configPath = "Data/SKSE/Plugins/RELight/Configs/" + meshPath + ".json";
                        newCfg.jsonIndex = 0;
                        newCfg.configID = globals::nextID++;
                        newCfg.diffuseColor = { 255, 162, 61 };
                        newCfg.startingFade = 3.0f;

                        auto a_root = selected->Get3D();
                        if (!a_root) {
                            logger::warn("Could not load this object's 3D cannnot attach light.");
                            break;
                        }

                        auto attachNode = a_root->AsNode();
                        if (!attachNode) {
                            logger::warn("Could not load this object's node cannnot attach light.");
                            break;
                        }

                        niLight = LightManager::AttachLight(newCfg, attachNode, selected, meshPath, selected->GetFormID(), attachedDebugMarker);
                        LightData::configIDToJsonCfg[newCfg.configID] = newCfg;

                        SKSE::GetTaskInterface()->AddTask([]() {
                            LightData::ResetTriLightCache();
                            });

                        step = AttachLightStep::Done;
                        break;
                    }

                    auto a_root = selected->Get3D();
                    if (!a_root) {
                        ImGuiMCP::Text("Could not load this object's 3D.");
                        break;
                    }

                    auto attachNode = a_root->AsNode();
                    if (!attachNode) {
                        ImGuiMCP::Text("Could not load this object's node.");
                        break;
                    }

                    if (previewRef != selected->GetFormID() || previewSelectedIndex != selectedIndex) {
                        previewRef = selected->GetFormID();
                        previewSelectedIndex = selectedIndex;

                        for (const auto& cfg : selectedCfgs) {
                            niLight = LightManager::AttachLight(cfg, attachNode, selected, meshPath, selected->GetFormID(), attachedDebugMarker);
                        }

                        SKSE::GetTaskInterface()->AddTask([]() {
                            LightData::ResetTriLightCache();
                            });

                        UpdateRefRootTransforms(selected);
                    }

                    std::string refIDandModName = BuildRefIDAndModName(selected);

                    RemoveFromIniExcludeRefID(selected, refIDandModName);

                    step = AttachLightStep::Done;
                }

                ImGuiMCP::SameLine();

                if (ImGuiMCP::Button("Cancel")) {
                    resetState();
                    step = AttachLightStep::SelectTarget;
                }

                break;
         }
        
        case AttachLightStep::Done:
        {
            centerNextItem(440.0f);
            ImGuiMCP::Text("Light attached, you can now edit the light in the light editor.");

            ImGuiMCP::Spacing();

            centerNextItem(170.0f);

            if (ImGuiMCP::Button("Confirm")) {

                if (multiLight) {
                    std::string finalMenuName = menuName + " " + std::to_string(entryCount + 1);

                    if (refLight) {
                        if (!AppendNewConfigEntryFromLight(
                            jsonFilePath,
                            static_cast<std::uint16_t>(entryCount),
                            finalMenuName,
                            niLight,
                            BuildRefIDAndModName(selected),
                            "",
                            newCfg,
                            true,
                            formID)) {
                            logger::error("Failed to append ref multi-light config");
                        }
                    }
                    else {
                        if (!AppendNewConfigEntryFromLight(
                            jsonFilePath,
                            static_cast<std::uint16_t>(entryCount),
                            finalMenuName,
                            niLight,
                            "",
                            matched,
                            newCfg,
                            false,
                            formID)) {
                            logger::error("Failed to append mesh multi-light config");
                        }

                        RefreshNearbyObjects(selected, meshPath);
                    }

                    globals::baseFormsWithAttachedLights.emplace(baseFormID);
                    step = AttachLightStep::SelectTarget;
                    break;
                }

                if (!createNewTemplate) {

                    if (selectedCfgs.empty()) {
                        logger::warn("Failed to update config file because no template was selected.");
                        resetState();
                        step = AttachLightStep::SelectTarget;
                        break;
                    }

                    const auto& filePath = selectedCfgs[0].configPath;

                    if (!refLight) {
                        if (AddMeshPathToAllEntries(filePath, meshPath)) {
                            logger::info("Added mesh path to existing template successfully");
                        }
                        else {
                            logger::warn("Failed to update config file.");
                        }

                        newCfg = selectedCfgs[0];
                        newCfg.meshPath = meshPath;
                        LightData::AddConfigToMaps(newCfg, refLight, selected->GetFormID());
                        globals::baseFormsWithAttachedLights.emplace(baseFormID);
                        RefreshNearbyObjects(selected, meshPath);
                        resetState();
                        break;
                    }

                    LightConfig refCfg = selectedCfgs[0];
                    refCfg.configPath = filePath;
                    refCfg.jsonIndex = static_cast<std::uint16_t>(CountJsonEntriesInFile(filePath));
                    refCfg.menuName = selectedCfgs[0].menuName;
                    refCfg.refFormIDAndModName = BuildRefIDAndModName(selected);
                    refCfg.meshPath.clear();

                    if (refCfg.refFormIDAndModName.empty()) {
                        logger::error("Failed to build ref ID for selected object");
                        resetState();
                        break;
                    }

                    if (!AppendNewConfigEntryFromLight(
                        refCfg.configPath,
                        refCfg.jsonIndex,
                        refCfg.menuName,
                        niLight,
                        refCfg.refFormIDAndModName,
                        "",
                        refCfg,
                        true,
                        selected->GetFormID())) {
                        logger::error("Failed to append ref-only config entry");
                        resetState();
                        break;
                    }

                    globals::baseFormsWithAttachedLights.emplace(baseFormID);
                    resetState();
                    break;
                }
                else {
                    if (!saveNewConfiguration(newCfg)) {
                        logger::error("Failed to save new template");
                    }

                    LightData::AddConfigToMaps(newCfg, refLight, selected->GetFormID());
                    globals::baseFormsWithAttachedLights.emplace(baseFormID);

                    if (!refLight) {
                        RefreshNearbyObjects(selected, meshPath);
                    }
                }

                step = AttachLightStep::SelectTarget;
                break;
            }

            ImGuiMCP::SameLine();

            if (ImGuiMCP::Button("Cancel")) {
                RE::ObjectRefHandle handle = selected->GetHandle();

                if (!selectedCfgs.empty()) {
                    LightData::configIDToJsonCfg.erase(newCfg.configID);
                }

                SKSE::GetTaskInterface()->AddTask([handle]() {
                    if (auto ref = handle.get()) {
                        ref->Disable();
                        ref->Enable(false);
                    }
                });

                resetState();
                step = AttachLightStep::SelectTarget;
                break;
            }

            break;
        }

        case AttachLightStep::LightRemoved:
        {
            centerNextItem(120.0f);
            ImGuiMCP::Text("Lights removed.");


            ImGuiMCP::Spacing();

            centerNextItem(50.0f);
            if (ImGuiMCP::Button("Okay")) {
                step = AttachLightStep::SelectTarget;
                break;
            }

            break;
        }
        }
    }


    bool saveSettingsToIni()
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
                    if (!inSection && line.find("; add esps by name to undisable their lights") != std::string::npos)
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

        outFile << "; enable light flicker prevention (default = false)\n";
        outFile << "enableLightFlickerPrevention=" << (globals::enableLightFlickerPreventionMeasures ? "true" : "false") << "\n\n";
        outFile << "; remove fake glow orbs (default = true)\n";
        outFile << "removeFakeGlowOrbs=" << (globals::removeFakeGlowOrbs ? "true" : "false") << "\n\n";
        outFile << "; enable debug bulbs (default = false)\n";
        outFile << "enableDebugBulbs=" << (globals::enableDebugLightBulbs ? "true" : "false") << "\n\n";
        outFile << "; ReLight INI\n";
        outFile << "; Logging Level (0: critical, 1: warnings/errors, 2: info, 3: debug)\n";
        outFile << "loggingLevel=" << globals::loggingLevel << "\n";
        outFile << "\n; Light merge settings\n";
        outFile << "light merge distance=" << globals::lightMergeDistance << "\n";
        outFile << "shadow light merge distance=" << globals::shadowLightMergeDistance << "\n";
        outFile << "light merge distance increased=" << globals::lightMergeSeekingDistance << "\n";
        outFile << "max z diff to merge=" << globals::fMaxZDiffToMerge << "\n";
        outFile << "max z diff to merge increased=" << globals::fMaxZDiffToMergeIncreased << "\n";
        outFile << "light fade increase per merge=" << globals::lightFadePerMerge << "\n";
        outFile << "light radius increase per merge=" << globals::lightRadiusPerMerge << "\n";
        outFile << "light fade max=" << globals::lightFadeMax << "\n";
        outFile << "light radius max=" << globals::lightRadiusMax << "\n";
        outFile << "light merge maxlights=" << globals::lightMergeMaxLights << "\n\n";

        // dump the entire preserved block back verbatim - comments, formids, everything
        if (!preservedBlock.empty())
            outFile << "\n" << preservedBlock;

        outFile.close();
        logger::info("ReLight.ini saved successfully!");
        return true;
    }
    //TODO:: clean and only use isl overlay if its installed
    void getAllLights() {
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

        // sort alphabetically
        std::sort(lights.begin(), lights.end(),
            [](const RE::NiPointer<RE::BSLight>& a,
                const RE::NiPointer<RE::BSLight>& b)
            {
                if (!a || !b) return false;

                const char* nameA = a->light->name.c_str();
                const char* nameB = b->light->name.c_str();

                if (!nameA || !nameB) return false;

                return std::strcmp(nameA, nameB) < 0;
            });
    }

    void restoreLightToDefaults(RE::NiPointer<RE::NiLight> light) {
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
}
