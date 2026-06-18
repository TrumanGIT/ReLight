#include "Menu.h"
#include "ticker.h"
#include "global.h"
#include "disableLights.h"
#include <format>
#include "everyFrame.h"

namespace logger = SKSE::log;

namespace UI {

	static RefreshTicker lightRefreshTicker(std::chrono::seconds(1));
    static buttonTicker saveButton{};
    static buttonTicker saveINIButton{};
    static buttonTicker defaultButton{};
    static buttonTicker deleteButton{};
    static vector<RE::NiPointer<RE::BSLight>> lights = {};
    static bool lightsLoaded = false;
    static bool enableLightEditor = false;
    static bool lightAlreadyInList = false;

    auto lightbulbIcon = FontAwesome::UnicodeToUtf8(0xf0eb);

    auto palletIcon = FontAwesome::UnicodeToUtf8(0xf53f); 

    auto coordinatesIcon = FontAwesome::UnicodeToUtf8(0xf601);

    auto editorIcon = FontAwesome::UnicodeToUtf8(0xf044);

    auto trashIcon = FontAwesome::UnicodeToUtf8(0xf1f8);

    auto plusIcon = FontAwesome::UnicodeToUtf8(0xf055);

    auto flagIcon = FontAwesome::UnicodeToUtf8(0xf024);

    void Register() {
        if (!SKSEMenuFramework::IsInstalled()) return;

        SKSEMenuFramework::SetSection("ReLight");

        SKSEMenuFramework::AddSectionItem("Settings", RenderSettings);

        SKSEMenuFramework::AddSectionItem("Light Editor", RenderLightEditor);

        SKSEMenuFramework::AddSectionItem("Attach Lights", RenderAttachRemove);

        SKSEMenuFramework::AddSectionItem("Light Merge", RenderLightMergeMenu);

        SKSEMenuFramework::AddSectionItem("Light Flicker Prevention", RenderLightFlickerPreventionMenu);
    }

    void __stdcall RenderLightFlickerPreventionMenu() {

        if (ImGuiMCP::BeginChild("Light Flicker Prevention", ImGuiMCP::ImVec2(0, 380), true,
            ImGuiMCP::ImGuiWindowFlags_NoScrollbar))
        {
            ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text, ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });

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

            ImGuiMCP::ImVec2 avail{};
            ImGuiMCP::GetContentRegionAvail(&avail);

            ImGuiMCP::Columns(2, "Bound", false);
            ImGuiMCP::SetColumnWidth(0, avail.x * 0.5f);
            ImGuiMCP::SetColumnWidth(1, avail.x * 0.5f);

            float colWidth = avail.x * 0.25f;
            ImGuiMCP::PushItemWidth(colWidth);

            if (ImGuiMCP::Checkbox("Enable Light flicker prevention", &globals::enableLightFlickerPreventionMeasures)) {
            }

            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("Only the 7 closest lights can affect a surface");
            }

            ImGuiMCP::SliderInt("Max Surface Size Flicker Prevention", &globals::largeSurfaceSize, 0, 5000);
            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("Surfaces larger than this will not participate in light flicker prevention (windhelm bridge is size 2300 and has many lights on it makes no sense to limit to only 7");
            }

            ImGuiMCP::SliderInt("Medium surface size", &globals::mediumSurfaceSize, 0, 1500);
            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("Distance checks are only enforced on surfaces (Trishape WorldBound) smaller than this");
            }

            ImGuiMCP::SliderInt("Small surface size", &globals::smallSurfaceSize, 0, 1000);
            if (ImGuiMCP::IsItemHovered()) { 
                ImGuiMCP::SetTooltip("Any surface (Trishape WorldBound) size larger will not have max light type per surface enforced on it");
            }
            ImGuiMCP::SliderInt("Candles Per SM Surface", &globals::maxCandlesPerSurfaceSM, 0, 7);
            if (ImGuiMCP::IsItemHovered())
            {
                ImGuiMCP::SetTooltip("Max candle lights allowed on small surfaces.");
            }

            ImGuiMCP::SliderInt("Chandeliers Per SM Surface", &globals::maxChandeliersPerSurfaceSM, 0, 7);
            if (ImGuiMCP::IsItemHovered())
            {
                ImGuiMCP::SetTooltip("Max chandelier lights allowed on small surfaces.");
            }

            ImGuiMCP::SliderInt("Fires Per SM Surface", &globals::maxFiresPerSurfaceSM, 0, 7);
            if (ImGuiMCP::IsItemHovered())
            {
                ImGuiMCP::SetTooltip("Max fire lights allowed on small surfaces.");
            }

            ImGuiMCP::NextColumn(); 
            ImGuiMCP::PushItemWidth(colWidth);

            ImGuiMCP::SliderInt("Candles Per M Surface", &globals::maxCandlesPerSurfaceM, 0, 10);
            if (ImGuiMCP::IsItemHovered())
            {
                ImGuiMCP::SetTooltip("Max candle lights allowed on medium surfaces.");
            }

            ImGuiMCP::SliderInt("Chandeliers Per M Surface", &globals::maxChandeliersPerSurfaceM, 0, 10);
            if (ImGuiMCP::IsItemHovered())
            {
                ImGuiMCP::SetTooltip("Max chandelier lights allowed on medium surfaces.");
            }

            ImGuiMCP::SliderInt("Fires Per M Surface", &globals::maxFiresPerSurfaceM, 0, 10);
            if (ImGuiMCP::IsItemHovered())
            {
                ImGuiMCP::SetTooltip("Max fire lights allowed on medium surfaces.");
            }

            ImGuiMCP::SliderFloat("Max Candle Distance", &globals::maxCandleDistance, 0, 1000);

            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("Max distance a candle can be to affect a medium and small surface (requires kCandle flag)");
            }

            ImGuiMCP::SliderFloat("Max Candle Z Distance", &globals::maxCandleZDistance, 0, 500);
            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("Maximum distance a candle can shine light on a medium and small surface below it (requires kCandle flag)");
            }

            ImGuiMCP::SliderFloat("Max Chandelier Distance", &globals::maxChandelierDistance, 0, 200);
            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("Max distance a chandelier can be to affect a medium and small surface (requires kChandelier flag)");
            }

            ImGuiMCP::SliderFloat("Max Chandelier Z Distance", &globals::maxChandelierZDistance, 0, 1000);
            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("Maximum vertical distance a chandelier can affect a medium and small surface (requires kChandelier flag)");
            }
  
            ImGuiMCP::PopItemWidth(); 
        }

        ImGuiMCP::EndChild();

    }

    void __stdcall RenderLightMergeMenu() {

        if (ImGuiMCP::BeginChild("Light Merge", ImGuiMCP::ImVec2(0, 340), true,
            ImGuiMCP::ImGuiWindowFlags_NoScrollbar))
        {
            ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text, ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });

            ImGuiMCP::Text("Light Merge");
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


            ImGuiMCP::ImVec2 avail{};
            ImGuiMCP::GetContentRegionAvail(&avail);

            ImGuiMCP::Columns(2, "Bound", false);
            ImGuiMCP::SetColumnWidth(0, avail.x * 0.5f);
            ImGuiMCP::SetColumnWidth(1, avail.x * 0.5f);

            float colWidth = avail.x * 0.25f;
            ImGuiMCP::PushItemWidth(colWidth);

            if (ImGuiMCP::Checkbox("Enable Light Merging", &globals::enableLightMerging)) {
            }

            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("Enable / Disable light merging");
            }

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

            ImGuiMCP::NextColumn();
            ImGuiMCP::PushItemWidth(colWidth);


            ImGuiMCP::SliderFloat("Z distance allowed to merge", &globals::fMaxZDiffToMerge, 0, 300);

            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("If z distance is greater, will not merge");
            }

            ImGuiMCP::SliderFloat("Z distance Increased", &globals::fMaxZDiffToMergeIncreased, 0, 300);

            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("For configs with the IncreasedMergeDistance flag");
            }

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

        ImGuiMCP::Checkbox("Draw Debug Lines", &globals::enableDebugLines);
        if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("Draw Lines Around Lights to make positioning easier");

        ImGuiMCP::SliderInt(
            "Max distance from light to draw debug lights",
            &globals::distanceForDrawDebugLines,
            0,
            10000,
            "%d");
        if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("any light futher then this value will not draw debug lines");
            

        ImGuiMCP::Separator();

        if (ImGuiMCP::SliderFloat(
            "Brightness Multiplier",
            &globals::brightnessModifier,
            0.1f,
            2.0f,
            "%.2f"))
        {
            auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
            if (ssNode) {
                auto& rt = ssNode->GetRuntimeData();

                auto applyBrightness = [](auto& lights) {
                    for (auto& light : lights) {
                        if (!light || !light->light)
                            continue;

                        auto& lightRt = light->light->GetLightRuntimeData();

                        const auto it = LightData::configIDToJsonCfg.find(lightRt.unk138);
                        if (it == LightData::configIDToJsonCfg.end())
                            continue;

                        const auto& cfg = it->second;

                        auto ref = light->light->GetUserData(); 

                        if (ref) {
                            lightRt.fade =
                                cfg.brightness *
                                ref->GetScale() *
                                globals::brightnessModifier;
                        }
                        else {
                            lightRt.fade =
                                cfg.brightness *
                                globals::brightnessModifier;
                        }
                    }
                    };

                applyBrightness(rt.activeLights);
                applyBrightness(rt.activeShadowLights);
            }
        }

        if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("Change brightness of all Relight lights.");

        if (ImGuiMCP::SliderFloat(
            "Non SKSE Lights Brightness Multiplier",
            &globals::vanillaBrightnessModifier,
            0.1f,
            2.0f,
            "%.2f")) {
        }

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

        ImGuiMCP::SameLine(0, 10.0f);

        bool deleteClicked =
            ImGuiMCP::Button(trashIcon.c_str());

        if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("Delete the Json file from Relight/Configs. You will need to restart the game for changes to take effect");

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
        if (deleteClicked) {
            if (selectedIndex >= 0 && selectedIndex < lights.size()) {
                ImGuiMCP::OpenPopup("Confirm Delete Light Template");
            }
            else {
                logger::warn("Delete clicked but no light selected");
                deleteButton.set(buttonState::Fail, 2.0f);
            }
        }

        if (ImGuiMCP::BeginPopupModal(
            "Confirm Delete Light Template",
            nullptr,
            ImGuiMCP::ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGuiMCP::Text("Are you sure you want to delete this light template?");

            ImGuiMCP::Spacing();

            if (ImGuiMCP::Button("Delete"))
            {
                deleteButton.set(buttonState::Working);

                bool ok = DeleteSelectedLightTemplate(selectedIndex, lights);

                deleteButton.set(
                    ok ? buttonState::Success : buttonState::Fail,
                    2.0f);

                ImGuiMCP::CloseCurrentPopup();
                return;
            }

            ImGuiMCP::SameLine();

            if (ImGuiMCP::Button("Cancel")) {
                ImGuiMCP::CloseCurrentPopup();
            }

            ImGuiMCP::EndPopup();
        }

        renderDone(saveButton, iconX, iconY);
        renderDone(defaultButton, iconX, iconY);

        ImGuiMCP::Separator();

        if (lightRefreshTicker.shouldTick()) {
            refreshAllLights(selectedIndex, lights);
            didRefreshThisFrame = !didRefreshThisFrame;
        }

        // catagorizies lights based on user defined catagories, alphabetizes ect
            RenderLightList(lights, selectedIndex);

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

                float radiusToUse = isSpotLight ? 5000.0f : 500.0f; 

                float brightnessToUse = isSpotLight ? 50.0f : 10.0f;

                bool isTorch = (selectedLight->light->name == "RLtorch");

                bool isShadowLight = config.shadowLight; 

                if (globals::enableDebugLines &&!(config.flags & static_cast<uint32_t>(LIGHT_FLAGS::kSpotLight))) {

                    globals::skseMenuOpened = true;      // menu is open this frame
                    globals::debugLinesNeedClear = true; 

                      auto player = RE::PlayerCharacter::GetSingleton();
                      if (!player) return;

                      auto playerPos = player->GetPosition();

                      auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                      if (!ssNode) {
                            logger::warn("ShadowSceneNode[0] is null!");
                            return;
                      }

                      auto& ssRt = ssNode->GetRuntimeData();

                      DrawLightDebugSpheres(ssRt.activeLights, playerPos, config.configID);

                      DrawLightDebugSpheres(ssRt.activeShadowLights, playerPos, config.configID);

                      DebugAPI_IMPL::DebugAPI::GetSingleton()->Update();
                }

                ImGuiMCP::PushID(selectedLight->light.get());

                ImGuiMCP::Dummy(ImGuiMCP::ImVec2(0.0f, 10.0f));

                // This is where the template editor starts.

                //////////////////////////////////////////////////////////////////////////////////////////////////////
                // We create a child for the other 4 main parameters (name, category, add flag, remove flag)
                //////////////////////////////////////////////////////////////////////////////////////////////////////

                if (ImGuiMCP::BeginChild("TemplateEditor", ImGuiMCP::ImVec2(0, 0), true,
                    ImGuiMCP::ImGuiWindowFlags_NoScrollbar))
                {

                    //////////////////////////////////////////////////////////////////////////////////////////////////
                    // Template Name
                    //////////////////////////////////////////////////////////////////////////////////////////////////

                    static char newTemplateName[255];

                    strncpy(newTemplateName, config.menuName.c_str(), sizeof(newTemplateName));
                    newTemplateName[sizeof(newTemplateName) - 1] = '\0';

                    ImGuiMCP::Text("Name:");
                    ImGuiMCP::SameLine();
                    ImGuiMCP::Dummy(ImGuiMCP::ImVec2(35.0f, 0.0f));
                    ImGuiMCP::SameLine();
                    ImGuiMCP::InputText("##templateName", newTemplateName, sizeof(newTemplateName));
                    if (ImGuiMCP::IsItemHovered()) {
                        ImGuiMCP::SetTooltip("This updates how the template name appears in the light editor.");
                    }

                    if (ImGuiMCP::IsItemDeactivatedAfterEdit() && config.menuName != newTemplateName)
                    {
                        config.menuName = newTemplateName;
                    }

                    /////////
                    //Flags
                   //////////

                    ImGuiMCP::SameLine(0.0f, 10.0f); 

                    static bool showFlagWindow = false;

                    if (ImGuiMCP::Button((std::string("Flags ") + flagIcon).c_str())) {
                        showFlagWindow = !showFlagWindow;
                    }

                    if (showFlagWindow) {
                        ImGuiMCP::Begin("Flag Window", &showFlagWindow, ImGuiMCP::ImGuiWindowFlags_::ImGuiWindowFlags_None);

                        static uint32_t lightFlags = 0;

                        auto FlagCheckbox = [](const char* label, uint32_t& flags, LIGHT_FLAGS flag, const char* tooltip)
                            {
                                bool checked = (flags & static_cast<uint32_t>(flag)) != 0;
                                if (ImGuiMCP::Checkbox(label, &checked))
                                {
                                    if (checked)
                                        flags |= static_cast<uint32_t>(flag);
                                    else
                                        flags &= ~static_cast<uint32_t>(flag);
                                }

                                if (ImGuiMCP::IsItemHovered())
                                {
                                    ImGuiMCP::SetTooltip("%s", tooltip);
                                }
                            };

                        FlagCheckbox("Candle", lightFlags, LIGHT_FLAGS::kCandle,
                            "Important for light flicker prevention. Also allows the light to merge with any other "
                            "light created from a relight json file which also has the Candle flag.");

                        FlagCheckbox("Chandelier", lightFlags, LIGHT_FLAGS::kChandelier,
                            "Important for light flicker prevention.");

                        FlagCheckbox("Fire", lightFlags, LIGHT_FLAGS::kFire,
                            "Important for light flicker prevention. Also allows the light to merge with any other "
                            "light created from a relight json file which also has the Fire flag.");

                        FlagCheckbox("Giant Campfire", lightFlags, LIGHT_FLAGS::kGiantCampfire,
                            "Allows merging with any other light created by a relight json config with the Fire flag.");

                        FlagCheckbox("Other", lightFlags, LIGHT_FLAGS::kOther,
                            "Allows merging with any other light created by a relight json config with the Other flag.");

                        FlagCheckbox("Increased Merge Distance", lightFlags, LIGHT_FLAGS::kIncreasedMergeDistance,
                            "Grants a significantly larger merge distance to lights. Currently used for ruin candles.");

                        FlagCheckbox("Increased Menu XYZ Scale", lightFlags, LIGHT_FLAGS::kIncreasedMenuXYZScale,
                            "Increases the in-game menu XYZ position slider range from 250 to 1250, for positioning "
                            "lights on larger objects.");

                        FlagCheckbox("No Merging", lightFlags, LIGHT_FLAGS::kNoMerging,
                            "Light sources of this type will never merge.");

                        FlagCheckbox("Outdoor", lightFlags, LIGHT_FLAGS::kOutdoor,
                            "Light source is to be applied outdoors only. Lets you have interior/exterior lighting "
                            "separated -- e.g. an Outdoor-flagged lantern vs a regular one with no flags.");


                        ImGuiMCP::End();
                    }

                    ImGuiMCP::Dummy(ImGuiMCP::ImVec2(0.0f, 10.0f));

                    //////////////////////////////////////////////////////////////////////////////////////////////////
                    // CATEGORY EDITOR
                    //////////////////////////////////////////////////////////////////////////////////////////////////

                    static char newTemplateCategory[255];

                    strncpy(newTemplateCategory, config.menuCategory.c_str(), sizeof(newTemplateCategory));
                    newTemplateCategory[sizeof(newTemplateCategory) - 1] = '\0';

                    ImGuiMCP::Text("Category:");
                    ImGuiMCP::SameLine();
                    ImGuiMCP::Dummy(ImGuiMCP::ImVec2(10.0f, 0.0f));
                    ImGuiMCP::SameLine();
                    ImGuiMCP::InputText("##templateCategory", newTemplateCategory, sizeof(newTemplateCategory));
                    if (ImGuiMCP::IsItemHovered()) {
                        ImGuiMCP::SetTooltip("This organizes templates into a dropdown for a cleaner layout.");
                    }

                    if (ImGuiMCP::IsItemDeactivatedAfterEdit() && config.menuCategory != newTemplateCategory)
                    {
                        config.menuCategory = newTemplateCategory;
                    }
    
                    ImGuiMCP::Dummy(ImGuiMCP::ImVec2(0.0f, 20.0f));

                }

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

                    if (ImGuiMCP::SliderFloat("Brightness", &config.startingFade, 0.0f, brightnessToUse, "%.1f")) {
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

                    ImGuiMCP::SameLine();
                    ImGuiMCP::PushStyleColor(
                        ImGuiMCP::ImGuiCol_Text,
                        ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });
                    ImGuiMCP::Text("Flicker");
                    ImGuiMCP::PopStyleColor();

                    ImGuiMCP::Separator();

                    ImGuiMCP::BeginDisabled(isTorch);

                    ImGuiMCP::SliderFloat(
                        "Flickers / Second",
                        &config.flickersPerSecond,
                        0.0f, 5.0f, "%.2f");

                    ImGuiMCP::BeginDisabled(config.flickersPerSecond == 0.0);

                    ImGuiMCP::SliderFloat(
                        "Flicker Intensity",
                        &config.flickerIntensity,
                        0.0f, 1.0f, "%.2f");
               
                    ImGuiMCP::SliderFloat(
                        "Movement",
                        &config.flickerAmplitude,
                        0.0f, 1.0f, "%.2f");
                    ImGuiMCP::EndDisabled();
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
                    if (isSpotLight && !isTorch)
                    {
                        if (ImGuiMCP::SliderFloat3(
                            "Rotation",
                            &config.rotation[0],
                            -180.0f,
                            180.0f,
                            "%.3f"))
                        {
                            auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];

                            if (ssNode)
                            {
                                auto& rt = ssNode->GetRuntimeData();

                                auto applyRotation = [&](auto& lights)
                                    {
                                        for (auto& l : lights)
                                        {
                                            if (!l)
                                                continue;

                                            if (l->light->GetLightRuntimeData().unk138 != lightData.unk138)
                                                continue;

                                            RE::NiMatrix3 rot;
                                            rot.SetEulerAnglesXYZ(
                                                RE::deg_to_rad(config.rotation[0]),
                                                RE::deg_to_rad(config.rotation[1]),
                                                RE::deg_to_rad(config.rotation[2])
                                            );

                                            l->light->local.rotate = rot;

                                            if (auto* parent = l->light->parent)
                                            {
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
                        RefreshNonRuntimeSettings(config);
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
                    if (ImGuiMCP::IsItemHovered()) {
                        ImGuiMCP::SetTooltip("For Spotlights");
                    }

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
                            
                            // set fov if its not lower then 45
                            if (config.fov > 45.0f) {
                                config.fov = 45.0f;
                            }
                         
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
        static std::string menuCategory{};
        static std::string menuName{};
        static std::string matched{};
        static RE::NiLight* niLight = nullptr;
        static RE::FormID lastSelected = 0;
        static RE::FormID previewRef = 0;
        static int previewSelectedIndex = -1;
        static std::vector<std::tuple<std::string, LightConfig, bool>> configDisplay;
        static std::unordered_set<std::string> seenMenuNames;
        static int selectedIndex = -1;
        static std::vector<LightConfig> selectedCfgs;
        static std::size_t entryCount = 0;

        static RE::TESObject* baseObject = nullptr;
        static RE::TESModel* model = nullptr;

        static char menuNameBuffer[128]{};
        static bool menuNameBufferInitialized = false;

        static LightConfig newCfg;

        auto resetState = [&]() {
            createNewTemplate = false;
            multiLight = false;
            refLight = false;
           attachedDebugMarker = false;

            meshPath.clear();
            jsonFilePath.clear();
            menuCategory.clear();
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

            menuNameBufferInitialized = false;
            menuNameBuffer[0] = '\0';
        };

        auto selected = RE::Console::GetSelectedRef().get();

        if (!selected) {
            resetState();
            ImGuiMCP::Dummy({ 0.0f, 50.0f });
            centerNextItem(350.0f);
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

            auto niAVObject = selected->Get3D();

            if (!niAVObject) return; 

            auto niNode = niAVObject->AsNode(); 

            if (!niNode) return; 

            step = LightManager::HasRelightLight(niNode) ?
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
                    menuCategory = multiLightCfg.menuCategory;
                    menuName = StripTrailingIdentifier(multiLightCfg.menuName);

                    auto root = selected->Get3D();

                    if (!root) break;

                    auto rootAsNode = root->AsNode();

                    if (!rootAsNode) break;

                    newCfg = multiLightCfg;
                    newCfg.configID = globals::nextID++;
                    newCfg.menuCategory = menuCategory;
                    newCfg.menuName = std::format("{} [{}]", menuName, entryCount);
                    logger::info("new menuName {}", newCfg.menuName);
                    if (!newCfg.menuCategory.empty()) {
                        logger::info("menuCategory applied is ", newCfg.menuCategory);
                    }

                    LightData::configIDToJsonCfg[newCfg.configID] = newCfg;
                    LightData::defaultConfigs[newCfg.configID] = newCfg;
               
                    niLight = LightManager::AttachLight(newCfg, rootAsNode, selected, meshPath, selected->GetFormID(), attachedDebugMarker);

                    refLight = true;
                    formID = selected->GetFormID();

                    SKSE::GetTaskInterface()->AddTask([]() {
                        LightData::ResetTriLightCache();
                    });

                    UpdateRefRootTransforms(selected);
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

                    menuCategory = selectedCfgs[0].menuCategory;
                    menuName = selectedCfgs[0].menuName;
                    jsonFilePath = selectedCfgs[0].configPath;        
                    entryCount = CountJsonEntriesInFile(selectedCfgs[0].configPath);


                    newCfg = selectedCfgs[0];

                    newCfg.menuCategory = menuCategory;

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

                    UpdateRefRootTransforms(selected);
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

                seenMenuNames.clear();

                auto tryAdd = [&](const std::string& key, const std::vector<LightConfig>& cfgVec, bool isExterior) {
                    if (cfgVec.empty()) return;
                    const auto& cfg = cfgVec[0];
                    std::string name = cfg.menuName.empty() ? key : cfg.menuName;
                    std::string nameLower = toLowerImmut(name);
                    if (seenMenuNames.insert(nameLower).second) {
                        configDisplay.push_back({ key, cfg, isExterior });
                    }
                    };

                for (auto& [key, cfgVec] : LightData::meshPathToJsonCfg)
                    tryAdd(key, cfgVec, false);
                for (auto& [key, cfgVec] : LightData::meshPathToJsonCfgExteriors)
                    tryAdd(key, cfgVec, true);

                std::sort(configDisplay.begin(), configDisplay.end(),
                    [](const auto& a, const auto& b)
                    {
                        return compareLightNames(
                            std::get<1>(a).menuName.c_str(),
                            std::get<1>(b).menuName.c_str()
                        );
                    });
            }

                for (int i = 0; i < static_cast<int>(configDisplay.size()); i++) {
                    const auto& [key, cfg, isExterior] = configDisplay[i];

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
                const auto& [selectedKey, selectedCfg, isExterior] = configDisplay[selectedIndex];
                selectedCfgs.clear();
                auto& map = isExterior
                    ? LightData::meshPathToJsonCfgExteriors
                    : LightData::meshPathToJsonCfg;
                if (auto it = map.find(selectedKey); it != map.end())
                    selectedCfgs = it->second;

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
            centerNextItem(290.0f);
            ImGuiMCP::Text("This object only, or all objects like it?");

            ImGuiMCP::Spacing();
            ImGuiMCP::Dummy({ 0.0f, 20.0f });

            centerNextItem(330.0f);

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

                        UpdateRefRootTransforms(selected);

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
            centerNextItem(520.0f);
            ImGuiMCP::Text("Light attached. You MUST confirm before saving in the light editor.");

            ImGuiMCP::Spacing();
            ImGuiMCP::Dummy({ 0.0f, 20.0f });

            bool showMenuNameBox =
                createNewTemplate &&
                !multiLight;

            if (showMenuNameBox && !menuNameBufferInitialized) {
                std::string initialName;

                if (!createNewTemplate) {
                    initialName = selectedCfgs[0].menuName;
                }
                else {
                    initialName = newCfg.menuName;
                }

                std::strncpy(menuNameBuffer, initialName.c_str(), sizeof(menuNameBuffer) - 1);
                menuNameBuffer[sizeof(menuNameBuffer) - 1] = '\0';

                menuNameBufferInitialized = true;
            }

            if (showMenuNameBox) {
                ImGuiMCP::SetCursorPosX(320.0f);
                ImGuiMCP::Text("Set Menu Name: ");
                ImGuiMCP::SameLine(); 
                ImGuiMCP::SetNextItemWidth(250.0f);

                ImGuiMCP::InputText(
                    "##TemplateName",
                    menuNameBuffer,
                    sizeof(menuNameBuffer));
            }

            ImGuiMCP::Dummy({ 0.0f, 20.0f });

            centerNextItem(180.0f);

            if (ImGuiMCP::Button("Confirm")) {

                if (multiLight) {
                    entryCount = CountJsonEntriesInFile(newCfg.configPath);
                    std::string finalMenuName =
                        std::format("{} [{}]", StripTrailingIdentifier(newCfg.menuName), entryCount); 
                    std::string finalMenuCategory = newCfg.menuCategory;

                    if (refLight) {
                        if (!AppendNewConfigEntryFromLight(
                            jsonFilePath,
                            static_cast<std::uint16_t>(entryCount),
                            finalMenuCategory,
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
                            finalMenuCategory,
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

                    // refid light
                    LightConfig refCfg = selectedCfgs[0];

                    std::string refFormIDAndModName = BuildRefIDAndModName(selected);
                    RemoveFromIniExcludeRefID(selected, refFormIDAndModName);
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
                    if (!multiLight) {
                        newCfg.menuName = menuNameBuffer;
                    }

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
