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

        SKSEMenuFramework::AddSectionItem("Light Flicker Prevention", UI::RenderTestingMenu);
    }

    void __stdcall RenderTestingMenu() {
        ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text, ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });

        FontAwesome::PushSolid();

        ImGuiMCP::Text("Light Flicker Prevention");
        ImGuiMCP::PopStyleColor();

        ImGuiMCP::Spacing();

        if (ImGuiMCP::Checkbox("enable Light flicker Preventin Measures", &globals::enableLightFlickerPreventionMeasures)) {
        }

        if (ImGuiMCP::IsItemHovered()) {
            ImGuiMCP::SetTooltip("sets global bounding boxes on light reach, 2 chandeliers per tri shape max");
        }

        ImGuiMCP::Spacing();

        if (ImGuiMCP::BeginChild("Light Bounds", ImGuiMCP::ImVec2(0, 250), true,
            ImGuiMCP::ImGuiWindowFlags_NoScrollbar))
        {
            ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text, ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });
            ImGuiMCP::Text("Light reach bounds");
            ImGuiMCP::PopStyleColor();

            ImGuiMCP::ImVec2 avail{};
            ImGuiMCP::GetContentRegionAvail(&avail);

            ImGuiMCP::Columns(2, "Bound", false);
            ImGuiMCP::SetColumnWidth(0, avail.x * 0.5f);   
            ImGuiMCP::SetColumnWidth(1, avail.x * 0.5f);

            float colWidth = avail.x * 0.25f;         
            ImGuiMCP::PushItemWidth(colWidth);

            ImGuiMCP::Spacing();

            ImGuiMCP::SliderFloat("Candle reach on tri shape##1", &globals::gMinCandleCoverage, 0.0f, 2000.0f);
            ImGuiMCP::SliderFloat("Candle reach on wall##1", &globals::minCandleCoverageWall, 0.0f, 2000.0f);
            ImGuiMCP::SliderFloat("Fire reach on tri shape##1", &globals::gMinFireCoverage, 0.0f, 2000.0f);
            ImGuiMCP::SliderFloat("Fire reach on wall##1", &globals::gMinFireCoverageWall, 0.0f, 2000.0f);

            ImGuiMCP::PopItemWidth();
            ImGuiMCP::NextColumn();
            ImGuiMCP::PushItemWidth(colWidth);      

            ImGuiMCP::SliderFloat("Max wall size strict bounds##2", &globals::maxWallSizeForStrictLightBounds, 0.0f, 500.0f);
            if (ImGuiMCP::IsItemHovered())
                ImGuiMCP::SetTooltip("Some walls like farmintinnwall are huge and don't need strict bounds");

            ImGuiMCP::SliderFloat("chandelier reach on tri shape##2", &globals::gMinChandelierCoverage, 0.0f, 2000.0f);
            ImGuiMCP::SliderFloat("global reach on tri shape##2", &globals::globalCoverage, 0.0f, 2000.0f);

            ImGuiMCP::PopItemWidth();
            ImGuiMCP::EndColumns();
        }
        ImGuiMCP::EndChild();
    
        ImGuiMCP::Spacing();

        if (ImGuiMCP::BeginChild("Light Merge", ImGuiMCP::ImVec2(0, 188), true,
            ImGuiMCP::ImGuiWindowFlags_NoScrollbar))
        {
            ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text, ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });

            ImGuiMCP::Text("Light Merge");
            ImGuiMCP::PopStyleColor();

            ImGuiMCP::SliderFloat("Max Z distance allowed to merge", &globals::fMaxZDiffToMerge, 0, 300);

            ImGuiMCP::SliderFloat("Distance of refs to light merge", &globals::lightMergeDistance, 0, 300);

            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("Sets max distance refs must be apart for them to merge into 1 light.");
            }

            ImGuiMCP::SliderFloat("Sets max distance to merge shadow light", &globals::shadowLightMergeDistance, 0, 300);

            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("Sets distance of refs to shadow light merge. (Don't turn off)");
            }

        }

        ImGuiMCP::EndChild();
        if (ImGuiMCP::Button("Debug log all lights")) {
            debugLogAllLights(); 
        }

        if (ImGuiMCP::Button("clear ref with attached lights set")) {
            globals::refsWithAttachedLights.clear();
        }

        if (ImGuiMCP::Button("Reinitialize Lights")) {
          
            clearSSNodeLights(); 

            auto player = RE::PlayerCharacter::GetSingleton();

            if (!player) return;

            LightManager::reinitializeLightsWithinRange(player);
        }

        if (ImGuiMCP::IsItemHovered()) {
            ImGuiMCP::SetTooltip("reset all lights within range of your skyrim pref. ini object lod setting");
        }

    }

    void __stdcall RenderSettings() {
        ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text, ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });

        FontAwesome::PushSolid();
        auto iconUtf8 = FontAwesome::UnicodeToUtf8(0xf0eb);

        ImGuiMCP::Text("%s ReLight Menu", iconUtf8.c_str());
        ImGuiMCP::PopStyleColor();
        ImGuiMCP::SameLine();

        if (ImGuiMCP::Button("Save INI")) {
            saveSettingsToIni();
        }
        if (ImGuiMCP::IsItemHovered())
            ImGuiMCP::SetTooltip("Write current settings to ReLight.ini");

        ImGuiMCP::Separator();

        ImGuiMCP::Checkbox("Remove Fake Glow Orbs", &globals::removeFakeGlowOrbs);
        if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("Remove fake glow orbs used by Bethesda");

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

        if (ImGuiMCP::CollapsingHeader("Excluded Nodes (Exact)")) {
            for (auto& entry : globals::exclusionList)
                ImGuiMCP::Text("%s", entry.c_str());
        }

        if (ImGuiMCP::CollapsingHeader("Excluded Nodes (Partial Match)")) {
            for (auto& entry : globals::exclusionListPartialMatch)
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

        // For debuggong
        //auto& rt = activeLight->light->GetLightRuntimeData();

        auto it = LightData::configIDToJsonCfg.find(key);
        if (it != LightData::configIDToJsonCfg.end()) {
            //auto& dataExt = it->second;
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

    bool didRefreshThisFrame = false;

    void __stdcall RenderLightEditor() {

        static int selectedIndex = -1;

        ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text, ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });

        FontAwesome::PushSolid();
     

        ImGuiMCP::Text("%s Light Editor", editorIcon.c_str());
        ImGuiMCP::PopStyleColor();
        ImGuiMCP::SameLine();

        //float rowY = ImGuiMCP::GetCursorPosY();

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
                auto selectedLight = lights[selectedIndex];
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

                //TODO:: shouldnet use node name lookups since we use mesh paths as well now.
                if (LightData::foundConfigForLight(niLight)) {
                    LightData::updateConfigFromLight(cfg, niLight);
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

        if (ImGuiMCP::Checkbox("Enable Editor", &enableLightEditor)) {
            if (enableLightEditor) {
                lightRefreshTicker.reset();
                refreshAllLights(selectedIndex);
            }
        }

        if (!enableLightEditor) {
            ImGuiMCP::Text("Light Editor is disabled. Enable it to edit light properties.");
            return;
        }

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

                //bool selected = (i == selectedIndex);

                auto lightName = removePrefix(light->light->name.c_str(), "RL");

                auto cfgs = findConfigsForNode(lightName);

                std::string menuName;

                for (auto& cfg : cfgs) {
                    if (light->light->GetLightRuntimeData().unk138 == cfg.configID)
                        menuName = cfg.menuName;
                }

                if (menuName.empty()) {
                    menuName = std::string(light->light->name.c_str());
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

                auto cfgs = findConfigsForNode(lightName);

                std::string menuName;

                for (auto& cfg : cfgs) {
                    if (light->light->GetLightRuntimeData().unk138 == cfg.configID)
                        menuName = cfg.menuName;
                }

                if (menuName.empty()) {
                    menuName = lightName;
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

            if (selectedIndex >= 0 && selectedIndex < lights.size()) {
                auto selectedLight = lights[selectedIndex];
                auto& lightData = selectedLight->light->GetLightRuntimeData();
                auto it = LightData::configIDToJsonCfg.find(lightData.unk138);
                if (it == LightData::configIDToJsonCfg.end())
                    return;

                auto& dataExt = it->second;

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
                float boxHeight = globals::islInstalled ? 200.0f : 150.0f;

                
                if (ImGuiMCP::BeginChild("BrightnessBox", ImGuiMCP::ImVec2(0, boxHeight), true,
                    ImGuiMCP::ImGuiWindowFlags_NoScrollbar))
                {
                    ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text,
                        ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });
                    FontAwesome::PushSolid();
      
                    ImGuiMCP::Text("%s Illuminance", lightbulbIcon.c_str());
                    ImGuiMCP::PopStyleColor();
                    ImGuiMCP::Separator();

                    if (ImGuiMCP::SliderFloat("Brightness", &dataExt.startingFade, 0.0f, 10.0f, "%.1f")) {
                        auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                        if (ssNode) {
                            auto& rt = ssNode->GetRuntimeData();
                            for (auto& l : rt.activeLights) {
                                if (!l) continue;
                                auto& rtData = l->light->GetLightRuntimeData();
                                if (rtData.unk138 == lightData.unk138)
                                    rtData.fade = dataExt.startingFade;
                            }
                            for (auto& l : rt.activeShadowLights) {
                                if (!l) continue;
                                auto& rtData = l->light->GetLightRuntimeData();
                                if (rtData.unk138 == lightData.unk138)
                                    rtData.fade = dataExt.startingFade;
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
                    ImGuiMCP::ImVec2(0, boxHeight),
                    true,
                    ImGuiMCP::ImGuiWindowFlags_NoScrollbar))
                {
                    // ----- Flicker Header -----

                    // ICON (dynamic)
                    if (didRefreshThisFrame && dataExt.flickersPerSecond != 0.0f) {
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
                        &dataExt.flickerIntensity,
                        0.0f, 1.0f, "%.2f");

                    ImGuiMCP::SliderFloat(
                        "Flickers / Second",
                        &dataExt.flickersPerSecond,
                        0.0f, 5.0f, "%.2f");
                    ImGuiMCP::EndDisabled();
                }
                ImGuiMCP::EndChild();
                

                ImGuiMCP::Columns(1);
                ImGuiMCP::Dummy(ImGuiMCP::ImVec2(0, 5));

               
                ImGuiMCP::Columns(2, nullptr, false);

               
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
                        &selectedLight->light->local.translate.x,
                        -250.0f, 250.0f, "%.3f"))
                    {
                        if (!isTorch) {
                            auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                            if (ssNode) {
                                auto& rt = ssNode->GetRuntimeData();
                                for (auto& l : rt.activeLights) {
                                    if (!l) continue;
                                    if (l->light->GetLightRuntimeData().unk138 != lightData.unk138) continue;
                                    l->light->local.translate = selectedLight->light->local.translate;
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
                                    l->light->local.translate = selectedLight->light->local.translate;
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

                if (ImGuiMCP::BeginChild("NonRuntimeBox", ImGuiMCP::ImVec2(0, 250), true,
                    ImGuiMCP::ImGuiWindowFlags_NoScrollbar))
                {

                    ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text,
                        ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });
                    ImGuiMCP::Text("Non-Runtime Light Settings");
                    ImGuiMCP::PopStyleColor();

                    ImGuiMCP::SameLine();

                    ImGuiMCP::Separator();

                    ImGuiMCP::ImVec2 avail{};
                    ImGuiMCP::GetContentRegionAvail(&avail);
                    float halfWidth = avail.x * 0.3f;

                    ImGuiMCP::Columns(2, "NonRuntimeColumns", false);

                    ImGuiMCP::Spacing();

                    ImGuiMCP::PushItemWidth(halfWidth);


                    if (ImGuiMCP::SliderFloat("Fall Off", &dataExt.falloff, 0.0f, 5.0f, "%.1f")) {
                    }

                    if (ImGuiMCP::SliderFloat("Constant", &dataExt.constAttenuation, 0.0f, 1.0f, "%.2f")) {
                    }

                    if (ImGuiMCP::SliderFloat("Linear", &dataExt.linearAttenuation, 0.0f, 1.0f, "%.2f")) {
                    }

                    if (ImGuiMCP::SliderFloat("Quadratic", &dataExt.quadraticAttenuation, 0.0f, 1.0f, "%.2f")) {
                    }

                    ImGuiMCP::NextColumn();

                    if (ImGuiMCP::SliderFloat("Depth Bias", &dataExt.depthBias, 0.0f, 1.0f, "%.2f")) {
                    }

                    if (ImGuiMCP::SliderFloat("FOV", &dataExt.fov, 0.0f, 180.0f, "%.2f")) {
                    }

                    if (ImGuiMCP::SliderFloat("Near Distance", &dataExt.nearDistance, 0.0f, 1.0f, "%.2f")) {
                    }

                    if (ImGuiMCP::Button("Refresh Lights")) {

                        clearSSNodeLights();

                        auto player = RE::PlayerCharacter::GetSingleton();

                        if (!player) {
                            logger::warn("no player character, couldent refresh lights");
                            return;
                        }

                        LightManager::reinitializeLightsWithinRange(player);
                    }

                    if (ImGuiMCP::IsItemHovered()) {
                        ImGuiMCP::SetTooltip("Changes to these settings require refreshing lights to take effect.");
                    }

                    ImGuiMCP::EndChild();
                }


                ImGuiMCP::PopID();
            }

        }

    }

    void saveSettingsToIni() {
        logger::info("Saving ReLight.ini...");

        const std::string path = "Data\\SKSE\\Plugins\\ReLight.ini";
        std::ofstream outFile(path, std::ios::trunc);

        if (!outFile.is_open()) {
            logger::error("Failed to open {} for writing!", path);
            return;
        }

        outFile << "; ReLight INI\n";
        outFile << "; Logging Level (0: critical, 1: warnings/errors, 2: info)\n";
        outFile << "loggingLevel=" << globals::loggingLevel << "\n\n";

        outFile << "; remove fake glow orbs (default = true)\n";
        outFile << "removeFakeGlowOrbs=" << (globals::removeFakeGlowOrbs ? "true" : "false") << "\n\n";

        outFile << "; add esps by name to undisable their lights (usually not needed)\n";
        outFile << "whitelist=";
        for (size_t i = 0; i < globals::whitelist.size(); i++) {
            outFile << globals::whitelist[i];
            if (i + 1 < globals::whitelist.size()) outFile << ",";
        }
        outFile << "\n\n";

        outFile << "; exclude specific nodes\n";
        for (auto& node : globals::exclusionList)
            outFile << node.c_str() << "\n";

        outFile << "\n; exclude partial nodes\n";
        for (auto& node : globals::exclusionListPartialMatch)
            outFile << node.c_str() << "\n";

        outFile << "\n; priority list (higher = first match. Usefull for candlechandelier ect to get correct lighting)\n";
        for (auto& node : globals::priorityList)
            outFile << node.c_str() << "\n";

        outFile.close();
        logger::info("ReLight.ini saved successfully!");
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

        lightData.radius = LightData::getNiPointLightRadius(backupCfg);
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

