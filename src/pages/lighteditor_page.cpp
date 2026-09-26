#include "pages/lighteditor_page.h"
#include "../ticker.h"
#include "../everyframe.h"

using namespace UI;

bool didRefreshThisFrame = false;

static RefreshTicker lightRefreshTicker(std::chrono::seconds(1));


void __stdcall RenderLightEditor() {

    static int relightSelectedIndex = -1;
    static int pluginSelectedIndex = -1;

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

    bool deleteClicked = ImGuiMCP::Button(trashIcon.c_str());
    if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("Delete the Json file from Relight/Configs. You will need to restart the game for changes to take effect");

    ImGuiMCP::ImVec2 rectMax = ImGuiMCP::GetItemRectMax();
    ImGuiMCP::ImVec2 rectMin = ImGuiMCP::GetItemRectMin();
    ImGuiMCP::ImVec2 winPos = ImGuiMCP::GetWindowPos();

    float iconX = (rectMax.x - winPos.x) + 10.0f;
    float iconY = (rectMin.y - winPos.y) + 4.0f;

    // resolve which list currently owns the selection, if any
    ActiveLightSelection active = ResolveActiveSelection(
        relightLights, relightSelectedIndex,
        pluginLights, pluginSelectedIndex);

    if (saveClicked) {

        bool ok = false;
        saveButton.set(buttonState::Working);

        if (active.valid()) {
            RE::NiPointer<RE::BSLight> selectedLight = active.get();
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

            // strip whichever prefix applies ("RL" or "PL")
            auto lightName = removePrefix(lightNameRL, "RL");

            LightConfig cfg;

            if (LightData::foundConfigForLightByConfigID(niLight)) {

                auto& baseConfig = LightData::configIDToJsonCfg[niLight->unk138];

                LightData::updateConfigFromLight(cfg, baseConfig, niLight);

                if (!LightData::updateRuntimeConfigCaches(cfg)) {
                    logger::warn("Failed to update runtime config caches for '{}'", lightName);
                }

                // Plugin light: create a new JSON config if one doesn't already exist
                if (baseConfig.isPluginLight) {

                    bool configExists =
                        !baseConfig.configPath.empty() &&
                        std::filesystem::exists(baseConfig.configPath);

                    if (!configExists) {
                        logger::info(
                            "Plugin light '{}' has no existing config file. Creating new configuration.",
                            lightName
                        );
                        saveNewConfiguration(cfg);
                        logger::info("created new json file for {} at path {} ", baseConfig.menuName, cfg.configPath);
                        ok = true;
                    }
                    else {
                        // Existing plugin config — save normally
                        saveConfiguration(cfg);
                        logger::info("Saving file for {} at path {} ", baseConfig.menuName, cfg.configPath);
                        ok = true;
                    }
                }
                else {
                    // Existing Relight / normal light
                    if (!cfg.configPath.empty()) {
                        saveConfiguration(cfg);
                        ok = true;
                    }
                    else {
                        logger::warn(
                            "Config for '{}' has no configPath, cannot save",
                            lightName
                        );
                        ok = false;
                    }
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

        if (active.valid()) {
            auto selectedLight = active.get();
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
        if (active.valid()) {
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

            bool ok = active.valid()
                ? DeleteSelectedLightTemplate(*active.index, *active.list)
                : false;

            deleteButton.set(ok ? buttonState::Success : buttonState::Fail, 2.0f);

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
        refreshAllLights(relightSelectedIndex, relightLights, "RL");
        refreshAllLights(pluginSelectedIndex, pluginLights, "ol");
        didRefreshThisFrame = !didRefreshThisFrame;
    }

    // ---------------------------------------------------------------------
    // LOADED TEMPLATES LIST  single list, toggled between plugin lights
    // and relight templates via showPluginLights
    // ---------------------------------------------------------------------

    static bool showPluginLights = false;

    // capture currently selected light before refresh (indices get reset on tick)
    RE::NiPointer<RE::BSLight> capturedSelected;
    if (relightSelectedIndex >= 0 && relightSelectedIndex < (int)relightLights.size())
        capturedSelected = relightLights[relightSelectedIndex];
    else if (pluginSelectedIndex >= 0 && pluginSelectedIndex < (int)pluginLights.size())
        capturedSelected = pluginLights[pluginSelectedIndex];

    if (lightRefreshTicker.shouldTick()) {
        refreshAllLights(relightSelectedIndex, relightLights, "RL");
        refreshAllLights(pluginSelectedIndex, pluginLights, "ol");
        didRefreshThisFrame = !didRefreshThisFrame;
    }

    // combine both lists every frame
    std::vector<RE::NiPointer<RE::BSLight>> allLights;
    allLights.reserve(relightLights.size() + pluginLights.size());
    allLights.insert(allLights.end(), relightLights.begin(), relightLights.end());
    allLights.insert(allLights.end(), pluginLights.begin(), pluginLights.end());

    // build filtered list every frame
    std::vector<RE::NiPointer<RE::BSLight>> filteredLights;
    filteredLights.reserve(allLights.size());
    for (const auto& l : allLights) {
        if (!l || !l->light) continue;
        auto configID = l->light->GetLightRuntimeData().unk138;
        auto it = LightData::configIDToJsonCfg.find(configID);
        bool isPlugin = it != LightData::configIDToJsonCfg.end() && it->second.isPluginLight;
        if (isPlugin == showPluginLights)
            filteredLights.push_back(l);
    }

    int displaySelectedIndex = -1;
    if (!filteredLights.empty() && capturedSelected) {
        for (int i = 0; i < (int)filteredLights.size(); ++i) {
            if (filteredLights[i] == capturedSelected) {
                displaySelectedIndex = i;
                break;
            }
        }
    }

    // toggle: plugin lights vs relight templates
    ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Button,
        showPluginLights ? ImGuiMCP::ImVec4{0.60F, 0.50F, 0.10F, 0.80F} : ImGuiMCP::ImVec4{0.35F, 0.35F, 0.35F, 0.5F});
    ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text,
        showPluginLights ? ImGuiMCP::ImVec4{1.0F, 0.95F, 0.9F, 1.0F} : ImGuiMCP::ImVec4{0.6F, 0.6F, 0.6F, 0.8F});
    if (ImGuiMCP::Button("Plugin Lights", ImGuiMCP::ImVec2(130, 0))) {
        if (!showPluginLights) {
            showPluginLights = true;
        }
    }

    if (ImGuiMCP::IsItemHovered()) {
        ImGuiMCP::SetTooltip("Any light from an ESP, ESL, or ESM plugin.");
    }

    ImGuiMCP::PopStyleColor(2);
    ImGuiMCP::SameLine();
    ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Button,
        !showPluginLights ? ImGuiMCP::ImVec4{0.60F, 0.50F, 0.10F, 0.80F} : ImGuiMCP::ImVec4{0.35F, 0.35F, 0.35F, 0.5F});
    ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text,
        !showPluginLights ? ImGuiMCP::ImVec4{1.0F, 0.95F, 0.9F, 1.0F} : ImGuiMCP::ImVec4{0.6F, 0.6F, 0.6F, 0.8F});
    if (ImGuiMCP::Button("Relight Lights", ImGuiMCP::ImVec2(130, 0))) {
        if (showPluginLights) {
            showPluginLights = false;
        }
    }

    if (ImGuiMCP::IsItemHovered()) {
        ImGuiMCP::SetTooltip("Lights attached using the Relight framework.");
    }

    ImGuiMCP::PopStyleColor(2);

    RenderLightList(filteredLights, displaySelectedIndex, "Loaded Lights");

    // translate post-render display index back to the correct original list
    if (displaySelectedIndex >= 0 && displaySelectedIndex < (int)filteredLights.size()) {
        auto& selected = filteredLights[displaySelectedIndex];
        if (!selected || !selected->light) {
            relightSelectedIndex = -1;
            pluginSelectedIndex = -1;
        } else {
            bool found = false;
            for (int i = 0; i < (int)pluginLights.size(); ++i) {
                if (pluginLights[i] == selected) {
                    pluginSelectedIndex = i;
                    relightSelectedIndex = -1;
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (int i = 0; i < (int)relightLights.size(); ++i) {
                    if (relightLights[i] == selected) {
                        relightSelectedIndex = i;
                        pluginSelectedIndex = -1;
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                relightSelectedIndex = -1;
                pluginSelectedIndex = -1;
            }
        }
    } else {
        relightSelectedIndex = -1;
        pluginSelectedIndex = -1;
    }

    // re-resolve after list rendering, since selection may have changed this frame
    active = ResolveActiveSelection(
        relightLights, relightSelectedIndex,
        pluginLights, pluginSelectedIndex);

    // ---------------------------------------------------------------------
    // SELECTED TEMPLATE SETTINGS
    // ---------------------------------------------------------------------
    if (active.valid()) {
        RE::NiPointer<RE::BSLight> selectedLight = active.get();

        if (!selectedLight || !selectedLight->light) {
            logger::warn("Selected light or BSLight is null, skipping light editor");
            return;
        }

        auto& lightData = selectedLight->light->GetLightRuntimeData();
        auto it = LightData::configIDToJsonCfg.find(lightData.unk138);

        if (it != LightData::configIDToJsonCfg.end()) {
            auto& config = it->second;

            Overlay* selectedIslRt = nullptr;
            bool islReady = true;

            auto selectedRef = selectedLight->light->GetUserData();

            if (globals::islInstalled) {
                selectedIslRt = Overlay::Get(selectedLight->light.get());
                if (!selectedIslRt) {
                    islReady = false;
                }
            }

            if (islReady) {
                static std::vector<std::pair<std::string, RE::TESRegion*>> regionList;

                if (regionList.empty()) {
                    BuildRegionList(regionList);
                }

                bool isSpotLight =
                    config.isPluginLight
                    ? (config.flags & static_cast<std::uint32_t>(RE::TES_LIGHT_FLAGS::kSpotlight) ||
                        config.flags & static_cast<std::uint32_t>(RE::TES_LIGHT_FLAGS::kSpotShadow))
                    : LightData::HasRelightFlag(config.flags, RELIGHT_FLAGS::kSpotLight);

                bool isPluginWithFlicker = config.isPluginLight &&
                    (config.flags & (static_cast<std::uint32_t>(RE::TES_LIGHT_FLAGS::kFlicker) |
                        static_cast<std::uint32_t>(RE::TES_LIGHT_FLAGS::kFlickerSlow) |
                        static_cast<std::uint32_t>(RE::TES_LIGHT_FLAGS::kPulse) |
                        static_cast<std::uint32_t>(RE::TES_LIGHT_FLAGS::kPulseSlow)));

                bool isPluginInverseSquare =
                    config.isPluginLight &&
                    (config.flags & static_cast<std::uint32_t>(TES_LIGHT_FLAGS_EXT::kInverseSquare));

                float radiusToUse = isSpotLight ? 5000.0f : 1000.0f;
                float brightnessToUse = isSpotLight ? 50.0f : 10.0f;
                bool isTorchOrMagicLight = selectedLight->light->fadeAmount == 4;
                bool isShadowLight = config.shadowLight;

                bool showISLSliders = globals::islInstalled &&
                    (isPluginInverseSquare ||
                        (!config.isPluginLight && (LightData::HasRelightFlag(config.flags, RELIGHT_FLAGS::kInverseSquare) || globals::allRelightsAsISL)));

                static bool showEmittanceWindow = false;

                if (ImGuiMCP::BeginChild("SelectedLightSettingsChild", ImGuiMCP::ImVec2(0, 680.0f), true))
                {
                    if (globals::enableDebugLines && !isSpotLight) {

                        auto player = RE::PlayerCharacter::GetSingleton();
                        auto* ssNode = player
                            ? RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0]
                            : nullptr;

                        if (player && ssNode) {
                            auto playerPos = player->GetPosition();
                            auto& ssRt = ssNode->GetRuntimeData();

                            DrawLightDebugSpheres(ssRt.activeLights, playerPos, config.configID);
                            DrawLightDebugSpheres(ssRt.activeShadowLights, playerPos, config.configID);

                            DebugAPI_IMPL::DebugAPI::GetSingleton()->Update();
                        }
                        else if (player && !ssNode) {
                            logger::warn("ShadowSceneNode[0] is null!");
                        }
                    }

                    ImGuiMCP::PushID(selectedLight->light.get());

                    //////////////////////////////////////////////////////////////////////////////////////////////////////
                    // TemplateEditor - Now dynamically sized based on content (height: 0 = auto-size)
                    //////////////////////////////////////////////////////////////////////////////////////////////////////

                    if (ImGuiMCP::BeginChild("TemplateEditor", ImGuiMCP::ImVec2(0, 0),
                        ImGuiMCP::ImGuiChildFlags_Border | ImGuiMCP::ImGuiChildFlags_AutoResizeY, ImGuiMCP::ImGuiWindowFlags_NoScrollbar))
                    {
                        static char newTemplateName[255];
                        strncpy(newTemplateName, config.menuName.c_str(), sizeof(newTemplateName));
                        newTemplateName[sizeof(newTemplateName) - 1] = '\0';

                        auto* style = ImGuiMCP::GetStyle();
                        constexpr float kButtonSpacing = 10.0f;

                        // --- Name row ---
                        std::string flagsLabel = std::string("Flags ") + flagIcon;
                        ImGuiMCP::ImVec2 flagsTextSize = ImGuiMCP::CalcTextSize(flagsLabel.c_str(), nullptr, false, -1.0f);

                        float flagsButtonWidth = flagsTextSize.x + style->FramePadding.x * 2.0f;

                        ImGuiMCP::Text("Name:");
                        ImGuiMCP::SameLine();
                        ImGuiMCP::Dummy(ImGuiMCP::ImVec2(35.0f, 0.0f));
                        ImGuiMCP::SameLine();

                        ImGuiMCP::ImVec2 avail = ImGuiMCP::GetContentRegionAvail();

                        float nameInputWidth = avail.x - flagsButtonWidth - kButtonSpacing;
                        if (nameInputWidth < 50.0f) {
                            nameInputWidth = 50.0f;
                        }

                        ImGuiMCP::SetNextItemWidth(nameInputWidth);
                        ImGuiMCP::InputText("##templateName", newTemplateName, sizeof(newTemplateName));

                        if (ImGuiMCP::IsItemHovered()) {
                            ImGuiMCP::SetTooltip(
                                "This updates how the template name appears in the light editor."
                            );
                        }

                        if (ImGuiMCP::IsItemDeactivatedAfterEdit() && config.menuName != newTemplateName) {
                            config.menuName = newTemplateName;
                        }


                        if (!config.isPluginLight) {
                            ImGuiMCP::SameLine(0.0f, kButtonSpacing);
                            RenderRelightFlags(config.flags);
                        }

                        ImGuiMCP::Dummy(ImGuiMCP::ImVec2(0.0f, 10.0f));

                        static char newTemplateCategory[255];
                        strncpy(newTemplateCategory, config.menuCategory.c_str(), sizeof(newTemplateCategory));
                        newTemplateCategory[sizeof(newTemplateCategory) - 1] = '\0';

                        // --- Category row ---
                        ImGuiMCP::ImVec2 emittanceTextSize = ImGuiMCP::CalcTextSize("External Emittance", nullptr, false, -1.0f);
                        float emittanceButtonWidth = emittanceTextSize.x + style->FramePadding.x * 2.0f;

                        ImGuiMCP::Text("Category:");
                        ImGuiMCP::SameLine();
                        ImGuiMCP::Dummy(ImGuiMCP::ImVec2(10.0f, 0.0f));
                        ImGuiMCP::SameLine();

                        ImGuiMCP::ImVec2 categoryAvail = ImGuiMCP::GetContentRegionAvail();
                        float categoryInputWidth = categoryAvail.x - emittanceButtonWidth - style->ItemSpacing.x;
                        if (categoryInputWidth < 50.0f) categoryInputWidth = 50.0f;

                        ImGuiMCP::SetNextItemWidth(categoryInputWidth);
                        ImGuiMCP::InputText("##templateCategory", newTemplateCategory, sizeof(newTemplateCategory));
                        if (ImGuiMCP::IsItemHovered()) {
                            ImGuiMCP::SetTooltip("This organizes templates into a dropdown for a cleaner layout.");
                        }

                        if (ImGuiMCP::IsItemDeactivatedAfterEdit() && config.menuCategory != newTemplateCategory)
                        {
                            config.menuCategory = newTemplateCategory;
                        }

                        if (!config.isPluginLight) {
                            ImGuiMCP::SameLine();
                            DrawExternalEmittanceSelector(
                                config,
                                selectedRef,
                                regionList,
                                showEmittanceWindow
                            );
                        }
                    }
                    ImGuiMCP::EndChild(); // TemplateEditor

                    ImGuiMCP::Dummy(ImGuiMCP::ImVec2(0, 5));

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
                            if (ssNode && !isPluginWithFlicker) {
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


                        if (!showISLSliders) {
                            if (ImGuiMCP::SliderFloat("Radius", &lightData.radius.x, 1.0f, radiusToUse, "%.2f")) {
                                auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                                if (ssNode && !config.isPluginLight) {
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

                        if (selectedIslRt && showISLSliders) {
                            if (ImGuiMCP::SliderFloat("Cutoff (ISL)", &selectedIslRt->cutoffOverride, 0.01f, 0.99f, "%.2f")) {
                                auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                                if (ssNode && !config.isPluginLight) {
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
                                if (ssNode && !config.isPluginLight) {
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

                    ImGuiMCP::EndChild(); // BrightnessBox
                    ImGuiMCP::NextColumn();

                    if (ImGuiMCP::BeginChild(
                        "FlickerBox",
                        ImGuiMCP::ImVec2(0, 200),
                        true,
                        ImGuiMCP::ImGuiWindowFlags_NoScrollbar))
                    {
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

                        ImGuiMCP::BeginDisabled(config.isPluginLight && !isPluginWithFlicker);

                        if (ImGuiMCP::SliderFloat(
                            "Flicker Rate",
                            &config.flickersPerSecond,
                            0.0f, 1.0f, "%.2f"))
                        {
                            if (config.isPluginLight) {
                                if (auto* light = LightData::GetTESObjectLightFromNiLight(selectedLight->light.get())) {
                                    light->data.flickerPeriodRecip = config.flickersPerSecond;
                                }
                            }
                        }

                        ImGuiMCP::BeginDisabled(config.flickersPerSecond == 0.0f);

                        if (ImGuiMCP::SliderFloat(
                            "Flicker Intensity",
                            &config.flickerIntensity,
                            0.0f, 1.0f, "%.2f"))
                        {
                            if (config.isPluginLight) {
                                if (auto* light = LightData::GetTESObjectLightFromNiLight(selectedLight->light.get())) {
                                    light->data.flickerIntensityAmplitude = config.flickerIntensity;
                                }
                            }
                        }


                        if (ImGuiMCP::SliderFloat(
                            "Movement",
                            &config.flickerAmplitude,
                            0.0f,
                            5,
                            "%.2f"))
                        {
                            if (config.isPluginLight) {
                                if (auto* light = LightData::GetTESObjectLightFromNiLight(selectedLight->light.get())) {
                                    light->data.flickerMovementAmplitude = config.flickerAmplitude;
                                }
                            }
                        }

                        ImGuiMCP::EndDisabled(); //  (config.flickersPerSecond == 0.0f)

                        ImGuiMCP::EndDisabled(); // (!isPluginWithFlicker)

                    }
                    ImGuiMCP::EndChild(); // FlickerBox

                    ImGuiMCP::Columns(1);
                    ImGuiMCP::Dummy(ImGuiMCP::ImVec2(0, 5));

                    ImGuiMCP::Columns(2, nullptr, false);

                    float sliderRange = !config.isPluginLight && LightData::HasRelightFlag(config.flags, RELIGHT_FLAGS::kIncreasedMenuXYZScale)
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

                        if (ImGuiMCP::SliderFloat3(
                            "Position",
                            &config.position[0],
                            -sliderRange, sliderRange, "%.3f")) {

                            if (!isPluginWithFlicker) {

                                auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                                if (ssNode) {
                                    auto& rt = ssNode->GetRuntimeData();
                                    for (auto& l : rt.activeLights) {
                                        if (!l) continue;
                                        if (l->light->GetLightRuntimeData().unk138 != lightData.unk138) continue;
                                        l->light->local.translate.x = config.position[0];
                                        l->light->local.translate.y = config.position[1];
                                        l->light->local.translate.z = config.position[2];

                                        // 3 free floats used to store merged light positions, needed for flicker calcs movement
                                        l->light->worldBound.center.x = config.position[0];
                                        l->light->worldBound.center.y = config.position[1];
                                        l->light->worldBound.center.z = config.position[2];

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

                                        // 3 free floats used to store merged light positions, needed for flicker calcs movement
                                        l->light->worldBound.center.x = config.position[0];
                                        l->light->worldBound.center.y = config.position[1];
                                        l->light->worldBound.center.z = config.position[2];

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

                        if (isSpotLight)
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
                    }
                    ImGuiMCP::EndChild(); // PositionBox

                    ImGuiMCP::NextColumn();

                    static bool colorPickerOpen = false;

                    auto ApplyRuntimeColor = [&](const RE::NiColor& runtimeColor)
                        {
                            lightData.diffuse = runtimeColor;

                            auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                            if (!ssNode)
                                return;

                            auto& rt = ssNode->GetRuntimeData();

                            for (auto& l : rt.activeLights)
                            {
                                if (!l || !l->light)
                                    continue;

                                if (l->light->GetLightRuntimeData().unk138 == lightData.unk138)
                                {
                                    l->light->GetLightRuntimeData().diffuse = runtimeColor;
                                }
                            }

                            for (auto& l : rt.activeShadowLights)
                            {
                                if (!l || !l->light)
                                    continue;

                                if (l->light->GetLightRuntimeData().unk138 == lightData.unk138)
                                {
                                    l->light->GetLightRuntimeData().diffuse = runtimeColor;
                                }
                            }
                        };

                    if (ImGuiMCP::BeginChild("ColorBox", ImGuiMCP::ImVec2(0, boxSize), true,
                        ImGuiMCP::ImGuiWindowFlags_NoScrollbar))
                    {
                        ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text,
                            ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });
                        ImGuiMCP::Text("%s Color (RGB)", palletIcon.c_str());
                        ImGuiMCP::PopStyleColor();
                        ImGuiMCP::Separator();

                        if (ImGuiMCP::SliderInt3("RGB", &config.diffuseColor[0], 0, 255))
                        {
                            RE::NiColor runtimeColor{
                                config.diffuseColor[0] / 255.0f,
                                config.diffuseColor[1] / 255.0f,
                                config.diffuseColor[2] / 255.0f
                            };

                            ApplyRuntimeColor(runtimeColor);
                        }

                        float colorPickerButtonHeight = ImGuiMCP::GetFrameHeight();
                        ImGuiMCP::SameLine();
                        if (ImGuiMCP::ColorButton(
                            "##ColorPreview",
                            ImGuiMCP::ImVec4(
                                config.diffuseColor[0] / 255.0f,
                                config.diffuseColor[1] / 255.0f,
                                config.diffuseColor[2] / 255.0f,
                                1.0f),
                            ImGuiMCP::ImGuiColorEditFlags_NoTooltip,
                            ImGuiMCP::ImVec2(
                                colorPickerButtonHeight,
                                colorPickerButtonHeight)))
                        {
                            colorPickerOpen = true;
                        }
                        if (ImGuiMCP::IsItemHovered())
                        {
                            ImGuiMCP::BeginTooltip();
                            ImGuiMCP::Text("Click to open the color picker");
                            ImGuiMCP::Separator();
                            ImGuiMCP::Text("R: %.3f", lightData.diffuse.red);
                            ImGuiMCP::Text("G: %.3f", lightData.diffuse.green);
                            ImGuiMCP::Text("B: %.3f", lightData.diffuse.blue);
                            ImGuiMCP::EndTooltip();
                        }
                    }
                    ImGuiMCP::EndChild(); // ColorBox

                    if (colorPickerOpen)
                    {
                        if (ImGuiMCP::Begin("Color Picker", &colorPickerOpen,
                            ImGuiMCP::ImGuiWindowFlags_AlwaysAutoResize |
                            ImGuiMCP::ImGuiWindowFlags_NoCollapse))
                        {
                            float color[4] = {
                                config.diffuseColor[0] / 255.0f,
                                config.diffuseColor[1] / 255.0f,
                                config.diffuseColor[2] / 255.0f,
                                1.0f
                            };

                            if (ImGuiMCP::ColorPicker4(
                                "##Picker",
                                color,
                                ImGuiMCP::ImGuiColorEditFlags_NoAlpha))
                            {
                                config.diffuseColor[0] = static_cast<int>(color[0] * 255.0f);
                                config.diffuseColor[1] = static_cast<int>(color[1] * 255.0f);
                                config.diffuseColor[2] = static_cast<int>(color[2] * 255.0f);

                                RE::NiColor runtimeColor{
                                    color[0],
                                    color[1],
                                    color[2]
                                };

                                ApplyRuntimeColor(runtimeColor);
                            }
                        }
                        ImGuiMCP::End();
                    }

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

                        ImGuiMCP::BeginDisabled(isTorchOrMagicLight);

                        if (ImGuiMCP::Button("Refresh Lights")) {

                            if (config.isPluginLight) {

                                LightData::updateRuntimeConfigCaches(config);

                                auto* ref = LightData::GetRefFromLight(selectedLight->light.get());
                                auto handle = ref->GetHandle();

                                if (handle) {

                                    handle.get()->Disable();

                                    //wait a frame before reattaching
                                    SKSE::GetTaskInterface()->AddTask([handle]() {
                                        if (auto ref = handle.get()) {
                                            ref->Enable(false);
                                            LightData::ResetTriLightCache();
                                        }
                                        });
                                }

                            }
                            else {
                                RefreshNonRuntimeSettings(config);
                            }
                        }

                        if (ImGuiMCP::IsItemHovered()) {
                            ImGuiMCP::SetTooltip("Changes to these settings require refreshing lights to take effect.");
                        }

                        ImGuiMCP::Separator();

                        ImGuiMCP::ImVec2 avail = ImGuiMCP::GetContentRegionAvail();
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

                        if (!config.isPluginLight) {
                            ImGuiMCP::Checkbox("Is Shadow Light", &config.shadowLight);

                            ImGuiMCP::BeginDisabled(!isShadowLight);

                            bool isSpot =
                                LightData::HasRelightFlag(config.flags, RELIGHT_FLAGS::kSpotLight);

                            if (ImGuiMCP::Checkbox("SpotLight", &isSpot))
                            {
                                if (isSpot) {
                                    config.flags |= static_cast<int>(RELIGHT_FLAGS::kSpotLight);

                                    if (config.fov > 45.0f) {
                                        config.fov = 45.0f;
                                    }
                                }
                                else {
                                    config.flags &= ~static_cast<int>(RELIGHT_FLAGS::kSpotLight);
                                    config.fov = 90.0f;
                                }
                            }

                            if (ImGuiMCP::IsItemHovered()) {
                                ImGuiMCP::SetTooltip("SpotLights only work for shadow lights, FOV and rotation can be used to edit them");
                            }

                            ImGuiMCP::EndDisabled(); // closes !isShadowLight
                        }
                        ImGuiMCP::EndDisabled(); // closes isTorch

                        if (config.isPluginLight) {
                            RenderTESLightFlags(config.flags);

                            DrawExternalEmittanceSelector(
                                config,
                                selectedRef,
                                regionList,
                                showEmittanceWindow
                            );
                        }
                    }
                    ImGuiMCP::EndChild(); // NonRuntimeBox

                    ImGuiMCP::PopID();
                }
                ImGuiMCP::EndChild(); // SelectedLightSettingsChild
            }
        }
    }
}