#include "Menu.h"
#include "ticker.h"
#include "global.h"
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

        ImGuiMCP::SameLine(); 

        if (ImGuiMCP::Button("Debug log all lights")) {
            debugLogAllLights();
        }

        
        ImGuiMCP::Separator();

        ImGuiMCP::Checkbox("Disable Game Lights", &globals::disableGameLights);
        if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("Disable all game lights except for those in exclude by light editor ID section in Relight.ini\n Used so the Relight Official light add on can start with a clean base");

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

        if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip(" Set Logging Level (0: critical, 1: warnings/errors, 2: info, 3: debug)");

        ImGuiMCP::Separator();

        if (ImGuiMCP::CollapsingHeader("Whitelist (by plugin name)")) {
            for (auto& entry : globals::whitelist)
                ImGuiMCP::Text("%s", entry.c_str());
        }

        if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("Mods whos esp name are in here will not have their lights disabled by relight no matter what.");

        if (ImGuiMCP::CollapsingHeader("Priority Nodes")) {
            for (auto& entry : globals::priorityList)
                ImGuiMCP::Text("%s", entry.c_str());
        }

        if (ImGuiMCP::IsItemHovered())   ImGuiMCP::SetTooltip(
            "Relight uses partial string matching (e.g. \"candle\" matches any candle mesh).\n"
            "This can cause unintended matches (e.g. \"candlechandelier01\").\n"
            "Meshes listed here take priority and override broader matches."
        );

        if (ImGuiMCP::CollapsingHeader("Excluded Mesh Paths (Exact)")) {
            for (auto& entry : globals::meshPathExclusionList)
                ImGuiMCP::Text("%s", entry.c_str());
        }

        if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip(
            "Relight uses partial string matching (e.g. \"candle\" matches any candle mesh).\n"
            "This can cause unintended matches, any mesh name here will be excluded from getting Relights\n"
        );


        if (ImGuiMCP::CollapsingHeader("Excluded Mesh Paths (Partial Match)")) {
            for (auto& entry : globals::meshPathExclusionListPartialMatch)
                ImGuiMCP::Text("%s", entry.c_str());
        }

        if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip(
            "Relight uses partial string matching (e.g. \"candle\" matches any candle mesh).\n"
            "Any mesh name that contains a word in this list will be excluded from getting Relights\n"
        );

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
            refreshAllLights(selectedIndex, lights);
            didRefreshThisFrame = !didRefreshThisFrame;
        }

        if (ImGuiMCP::CollapsingHeader("Loaded Light Templates")) {

            std::unordered_map<std::string, std::vector<int>> groupedLights;

            for (int i = 0; i < lights.size(); i++) {
                auto& light = lights[i];
                if (!light || !light->light)
                    continue;

                auto configID = light->light->GetLightRuntimeData().unk138;

                auto it = LightData::configIDToJsonCfg.find(configID);
                if (it == LightData::configIDToJsonCfg.end())
                    continue;

                std::string groupName = it->second.configPath;

                if (groupName.empty())
                    groupName = "Unknown Config";

                groupedLights[groupName].push_back(i);
            }

            std::unordered_map<std::string, int> nameCounts;

            for (auto& [configPath, indices] : groupedLights) {
                for (int i : indices) {
                    auto& light = lights[i];
                    if (!light || !light->light)
                        continue;

                    auto configID = light->light->GetLightRuntimeData().unk138;

                    auto it = LightData::configIDToJsonCfg.find(configID);
                    if (it == LightData::configIDToJsonCfg.end())
                        continue;

                    std::string menuName = it->second.menuName;

                    if (menuName.empty() && !it->second.meshPaths.empty())
                        menuName = it->second.meshPaths[0];

                    nameCounts[menuName]++;
                }
            }

            std::vector<std::string> sortedConfigPaths;

            for (auto& [configPath, indices] : groupedLights) {
                sortedConfigPaths.push_back(configPath);
            }

            std::sort(sortedConfigPaths.begin(), sortedConfigPaths.end(),
                [&](const std::string& a, const std::string& b)
                {
                    auto getGroupMenuName = [&](const std::string& configPath) -> std::string {
                        auto& indices = groupedLights[configPath];

                        if (indices.empty())
                            return "";

                        auto& light = lights[indices.front()];

                        if (!light || !light->light)
                            return "";

                        auto configID = light->light->GetLightRuntimeData().unk138;

                        auto it = LightData::configIDToJsonCfg.find(configID);
                        if (it == LightData::configIDToJsonCfg.end())
                            return "";

                        if (!it->second.menuName.empty())
                            return it->second.menuName;

                        if (!it->second.meshPaths.empty())
                            return it->second.meshPaths[0];

                        return "";
                        };

                    return compareLightNames(
                        getGroupMenuName(a).c_str(),
                        getGroupMenuName(b).c_str()
                    );
                });

            for (auto& configPath : sortedConfigPaths) {
                auto& indices = groupedLights[configPath];

                if (indices.empty())
                    continue;

                int firstIndex = indices.front();
                auto& firstLight = lights[firstIndex];

                if (!firstLight || !firstLight->light)
                    continue;

                auto firstConfigID = firstLight->light->GetLightRuntimeData().unk138;

                auto firstIt = LightData::configIDToJsonCfg.find(firstConfigID);
                if (firstIt == LightData::configIDToJsonCfg.end())
                    continue;

                std::string groupMenuName = firstIt->second.menuName;

                if (groupMenuName.empty()) {
                    if (!firstIt->second.meshPaths.empty())
                        groupMenuName = firstIt->second.meshPaths[0];
                    else
                        groupMenuName = "Unknown Light";
                }

                if (nameCounts[groupMenuName] > 1 && !firstIt->second.meshPaths.empty()) {
                    groupMenuName += " (" + firstIt->second.meshPaths[0] + ")";
                }

                std::size_t jsonCount =
                    CountJsonEntriesInFile(firstIt->second.configPath);

                if (jsonCount > 1) {
                    std::string headerName =
                        groupMenuName + " (" + std::to_string(indices.size()) + ")";

                    ImGuiMCP::PushID(configPath.c_str());

                    if (ImGuiMCP::TreeNode(headerName.c_str())) {
                        for (int i : indices) {
                            auto& light = lights[i];
                            if (!light || !light->light)
                                continue;

                            bool selected = (i == selectedIndex);

                            auto configID = light->light->GetLightRuntimeData().unk138;

                            std::string menuName;

                            auto it = LightData::configIDToJsonCfg.find(configID);
                            if (it != LightData::configIDToJsonCfg.end()) {
                                menuName = it->second.menuName;

                                if (menuName.empty()) {
                                    if (!it->second.meshPaths.empty())
                                        menuName = it->second.meshPaths[0];
                                    else
                                        menuName = "Unknown Light";
                                }

                                if (nameCounts[menuName] > 1 && !it->second.meshPaths.empty()) {
                                    menuName += " (" + it->second.meshPaths[0] + ")";
                                }
                            }
                            else {
                                menuName = "Unknown Light";
                            }

                            ImGuiMCP::PushID(i);

                            if (ImGuiMCP::Selectable(menuName.c_str(), selected)) {
                                selectedIndex = i;
                            }

                            ImGuiMCP::PopID();
                        }

                        ImGuiMCP::TreePop();
                    }

                    ImGuiMCP::PopID();
                }
                else {
                    int i = firstIndex;

                    bool selected = (i == selectedIndex);

                    ImGuiMCP::PushID(i);

                    if (ImGuiMCP::Selectable(groupMenuName.c_str(), selected)) {
                        selectedIndex = i;
                    }

                    ImGuiMCP::PopID();
                }
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

                bool isSpotLight = config.flags & static_cast<uint32_t>(LIGHT_FLAGS::kSpotLight);

                float radiusToUse = isSpotLight ? 1000.0f : 500.0f; 

                bool isTorch = (selectedLight->light->name == "RLtorch");

                bool isShadowLight = config.shadowLight; 

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
                        if (ImGuiMCP::SliderFloat("Radius", &lightData.radius.x, 1.0f, radiusToUse, "%.2f")) {
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

                float boxSize = isSpotLight ? 150.0f : 100.0f;

                if (ImGuiMCP::BeginChild(
                    "PositionBox",
                    ImGuiMCP::ImVec2(0, boxSize),
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
                  if (isSpotLight){
                    if (ImGuiMCP::SliderFloat3(
                        "Rotation",
                        &config.rotation[0],
                        -180.0f,
                        180.0f,
                        "%.3f"))
                    {
                        if (!isTorch) {
                            auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];

                            if (ssNode) {
                                auto& rt = ssNode->GetRuntimeData();

                                auto applyRotation = [&](auto& lights)
                                    {
                                        for (auto& l : lights) {
                                            if (!l) continue;

                                            if (l->light->GetLightRuntimeData().unk138 != lightData.unk138)
                                                continue;

                                            RE::NiMatrix3 rot;

                                            rot.SetEulerAnglesXYZ(
                                                RE::deg_to_rad(config.rotation[0]),
                                                RE::deg_to_rad(config.rotation[1]),
                                                RE::deg_to_rad(config.rotation[2])
                                            );

                                            l->light->local.rotate = rot;

                                            if (auto* parent = l->light->parent) {
                                                RE::NiUpdateData updateData{};
                                                updateData.time = 0.0f;
                                                updateData.flags = RE::NiUpdateData::Flag::kDirty;

                                                parent->UpdateTransformAndBounds(updateData);
                                            }
                                        }
                                    };

                                applyRotation(rt.activeLights);
                                applyRotation(rt.activeShadowLights);
                            }
                        }
                    }
                  }

                    ImGuiMCP::EndDisabled();
                }
                ImGuiMCP::EndChild();

                ImGuiMCP::NextColumn();


                if (ImGuiMCP::BeginChild("ColorBox", ImGuiMCP::ImVec2(0, boxSize), true,
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

                if (ImGuiMCP::BeginChild("NonRuntimeBox", ImGuiMCP::ImVec2(0, 205), true,
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
                    if (ImGuiMCP::IsItemHovered()) {
                        ImGuiMCP::SetTooltip("Affect shadow quality");
                    }

                    ImGuiMCP::SliderFloat("FOV", &config.fov, 0.0f, 90.0f, "%.2f");
                    ImGuiMCP::NextColumn();
 
                    ImGuiMCP::SliderFloat("Near Distance", &config.nearDistance, 0.0f, 5.0f, "%.2f");
                    ImGuiMCP::Checkbox("Is Shadow Light", &config.shadowLight);

                    ImGuiMCP::BeginDisabled(!isShadowLight);

                    bool isSpot =
                        (config.flags & static_cast<int>(LIGHT_FLAGS::kSpotLight)) != 0;

                    if (ImGuiMCP::Checkbox("SpotLight", &isSpot))
                    {
                        if (isSpot) {
                            config.flags |= static_cast<int>(LIGHT_FLAGS::kSpotLight);
                            config.fov = 45.0f;
                        }
                        else {
                            config.flags &= ~static_cast<int>(LIGHT_FLAGS::kSpotLight);
                            config.fov = 90.0f;
                        }
                    }

                    if (ImGuiMCP::IsItemHovered()) {
                        ImGuiMCP::SetTooltip("SpotLights only work for shadow lights, FOV and rotation can be used to edit them");
                    }

                    ImGuiMCP::EndDisabled(); // closes !isShadowLight

                    ImGuiMCP::EndDisabled(); // closes isTorch

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

    void __stdcall RenderAttachRemove()
    {

        auto centerNextItem = [&](float estimatedWidth) { 
            float startX = ImGuiMCP::GetCursorPosX(); 
            
            ImGuiMCP::ImVec2 avail{}; ImGuiMCP::GetContentRegionAvail(&avail); 
            
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
            centerNextItem(350.0f);
            ImGuiMCP::Dummy({ 0.0f, 170.0f });
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
            ImGuiMCP::Dummy({ 0.0f, 50.0f });
            centerNextItem(470.0f);
            ImGuiMCP::Text("Object Selected in the console already has a ReLight light.");
      
            ImGuiMCP::Spacing(); 
            ImGuiMCP::Dummy({ 0.0f, 20.0f });
            centerNextItem(400.0f);

            if (ImGuiMCP::Button("Add another Light")) {

                multiLight = true;

                auto multiLightCfg = FindRefIDConfigForAttachAnother(selected);

                if (!multiLightCfg.configPath.empty()) {
                    logger::info("Add another light: found existing ref ID config {:08X}", selected->GetFormID());

                    entryCount = CountJsonEntriesInFile(multiLightCfg.configPath);
                    jsonFilePath = multiLightCfg.configPath;
                    menuName = StripTrailingIdentifier(multiLightCfg.menuName);

                    auto root = selected->Get3D();

                    if (!root) break;

                    auto rootAsNode = root->AsNode();

                    if (!rootAsNode) break;

                    newCfg = multiLightCfg;
                    newCfg.configID = globals::nextID++;

                    newCfg.menuName = std::format("{} [{}]", menuName, entryCount);
                    logger::info("new menuName {}", newCfg.menuName);

                    LightData::configIDToJsonCfg[newCfg.configID] = newCfg;
                    LightData::defaultConfigs[newCfg.configID] = newCfg;
               
                    niLight = LightManager::AttachLight(newCfg, rootAsNode, selected, meshPath, selected->GetFormID(), attachedDebugMarker);


                    refLight = true;
                    formID = selected->GetFormID();

                    SKSE::GetTaskInterface()->AddTask([]() {
                        LightData::ResetTriLightCache();
                        });
                }

                // else its a mesh path 
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


                    newCfg = selectedCfgs[0];


                    std::string finalMenuName =
                        std::format("{} [{}]", StripTrailingIdentifier(newCfg.menuName), entryCount);

                    newCfg.menuName = finalMenuName; 
                    newCfg.configID = globals::nextID++;

                    auto root = selected->Get3D(); 

                    if (!root) break; 

                    auto rootAsNode = root->AsNode(); 
                    
                    if (!rootAsNode) break; 

                    niLight = LightManager::AttachLight(newCfg, rootAsNode, selected, meshPath, selected->GetFormID(), attachedDebugMarker);
            
                    LightData::configIDToJsonCfg[newCfg.configID] = newCfg;
                    LightData::defaultConfigs[newCfg.configID] = newCfg;

                    SKSE::GetTaskInterface()->AddTask([]() {
                        LightData::ResetTriLightCache();
                        });
                }

                step = AttachLightStep::Done;
                break;
            }

            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("You can edit new light in Light Editor as Torch [1], Torch [2] ect.");
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
            ImGuiMCP::Dummy({ 0.0f, 50.0f });
            centerNextItem(430.0f);       
            ImGuiMCP::Text("      Attaching light to object selected in console.\nCreate new light template or add to existing template?");


            ImGuiMCP::Spacing();
            ImGuiMCP::Dummy({ 0.0f, 20.0f});
            centerNextItem(630.0f);
                                  
            if (ImGuiMCP::Button("Add to a existing template")) {
                createNewTemplate = false;
                step = AttachLightStep::ChooseTemplate;
            }

            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("Adding to a existing template keeps your config folder uncluttered.");
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

                // alphabetize
                std::sort(configDisplay.begin(), configDisplay.end(),
                    [](const auto& a, const auto& b)
                    {
                        return compareLightNames(
                            a.second.menuName.c_str(),
                            b.second.menuName.c_str()
                        );
                    });
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
                logger::error("Selected config was empty or not found.");
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
            ImGuiMCP::Dummy({ 0.0f, 50.0f });
            centerNextItem(260.0f);
            ImGuiMCP::Text("This object only, or all objects like it?");

            ImGuiMCP::Spacing();
            ImGuiMCP::Dummy({ 0.0f, 20.0f });

            centerNextItem(300.0f);

            if (ImGuiMCP::Button("This object only")) {

                refLight = true;

                if (createNewTemplate) {

                    auto refFormIDandModName = BuildRefIDAndModName(selected);

                    newCfg.refFormIDsAndModNames.push_back(refFormIDandModName);
                    newCfg.menuName = refFormIDandModName;
                    newCfg.configPath = BuildConfigPath(refFormIDandModName);
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

                    UpdateRefRootTransforms(selected);

                    SKSE::GetTaskInterface()->AddTask([]() {
                        LightData::ResetTriLightCache();
                    });


                    RemoveFromIniExcludeRefID(selected, refFormIDandModName);
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

                        newCfg.meshPaths.push_back(meshPath);
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
            ImGuiMCP::Dummy({ 0.0f, 50.0f });
            centerNextItem(495.0f);
            ImGuiMCP::Text("Light attached. You MUST confirm before saving in the light editor.");

            ImGuiMCP::Spacing();
            ImGuiMCP::Dummy({ 0.0f, 20.0f });
            centerNextItem(170.0f);

            if (ImGuiMCP::Button("Confirm")) {

                if (multiLight) {
                    entryCount = CountJsonEntriesInFile(newCfg.configPath);
                    std::string finalMenuName =
                        std::format("{} [{}]", StripTrailingIdentifier(newCfg.menuName), entryCount); 

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
                            formID, true)) {
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

                        // disable original otherwise duplicate light on original
                        selected->Disable();
                        selected->Enable(false);
                       
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
                        if (std::find(
                            newCfg.meshPaths.begin(),
                            newCfg.meshPaths.end(),
                            meshPath) == newCfg.meshPaths.end())
                        {
                            newCfg.meshPaths.push_back(meshPath);
                        }
                        LightData::AddConfigToMaps(newCfg, refLight, selected->GetFormID());
                        globals::baseFormsWithAttachedLights.emplace(baseFormID);
                        RefreshNearbyObjects(selected, meshPath);
                        resetState();
                        break;
                    }

                    LightConfig refCfg = selectedCfgs[0];

                    std::string refFormIDAndModName = BuildRefIDAndModName(selected);
                    refCfg.configPath = filePath;
                    refCfg.jsonIndex = static_cast<std::uint16_t>(CountJsonEntriesInFile(filePath));
                    refCfg.menuName = selectedCfgs[0].menuName;
                    refCfg.refFormIDsAndModNames.push_back(refFormIDAndModName);
                    refCfg.meshPaths.clear();

                    if (refCfg.refFormIDsAndModNames.empty()) {
                        logger::error("Failed to build ref ID for selected object");
                        resetState();
                        break;
                    }

                    AddRefIDToAllEntries(refCfg.configPath, refFormIDAndModName);

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

                if (multiLight) {
                    LightData::configIDToJsonCfg.erase(newCfg.configID);
                    LightData::defaultConfigs.erase(newCfg.configID);
                }

                resetState();
                step = AttachLightStep::SelectTarget;
                break;
            }

            break;
        }

        case AttachLightStep::LightRemoved:
        {
            ImGuiMCP::Dummy({ 0.0f, 50.0f });
            centerNextItem(120.0f);
            ImGuiMCP::Text("Lights removed.");

            ImGuiMCP::Dummy({ 0.0f, 20.0f });
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
}
