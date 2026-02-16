#include "Menu.h"
#include "global.h"
#include "Utility.h"
#include "lightManager.h"
#include <chrono>

namespace logger = SKSE::log;


//TODO:: Make default button also update parent transforms


namespace UI {

    // Based on Log Watcher state machine.
    enum class buttonState {
        Idle,
        Working,
        Success,
        Fail
    };

	struct buttonTransition {
        buttonState state{ buttonState::Idle };
        std::chrono::steady_clock::time_point until{};

        void set(const buttonState& s, const float& seconds = 2.0f) {
            state = s;
            if (s == buttonState::Success || s == buttonState::Fail) {
                until = std::chrono::steady_clock::now() + std::chrono::milliseconds((int)(seconds * 1000.0f));
            }
        }

        void tick() {
            if ((state == buttonState::Success || state == buttonState::Fail) && std::chrono::steady_clock::now() >= until) {
                state = buttonState::Idle;
            }
        }
    };

	void renderDone(buttonTransition& t, const float& x, const float& y) {

		t.tick();

		if (t.state == buttonState::Idle) return;

        ImGuiMCP::ImVec2 oldPos;
        ImGuiMCP::GetCursorPos(&oldPos);
        ImGuiMCP::SetCursorPos({ x, y });

		FontAwesome::PushSolid();
		if (t.state == buttonState::Working) {
			// We can show some pregress here if we want.
		}
		else if (t.state == buttonState::Success) {
			ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text, ImGuiMCP::ImVec4{ 0.2f, 1.0f, 0.2f, 1.0f }); // green
			auto s = FontAwesome::UnicodeToUtf8(0xf00c);
			ImGuiMCP::Text("%s", s.c_str());
			ImGuiMCP::PopStyleColor();
		}
		else { // Fail
			ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text, ImGuiMCP::ImVec4{ 1.0f, 0.2f, 0.2f, 1.0f }); // red
			auto s = FontAwesome::UnicodeToUtf8(0xf00d);
			ImGuiMCP::Text("%s", s.c_str());
			ImGuiMCP::PopStyleColor();
		}
		FontAwesome::Pop();

		ImGuiMCP::SetCursorPos(oldPos);
	}

    static buttonTransition saveButton{};
    static buttonTransition defaultButton{};
    static vector<RE::NiPointer<RE::BSLight>> lights = {};
    static bool lightsLoaded = false;
    static bool enableLightEditor = false;
    static bool lightAlreadyInList = false;

    void Register() {
        if (!SKSEMenuFramework::IsInstalled()) return;

        SKSEMenuFramework::SetSection("ReLight");

        SKSEMenuFramework::AddSectionItem("Settings", UI::RenderSettings);

        SKSEMenuFramework::AddSectionItem("Light Editor", UI::RenderLightEditor);

        SKSEMenuFramework::AddSectionItem("Testing", UI::RenderTestingMenu);
    }

    void __stdcall RenderTestingMenu() {
        ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text, ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });

        FontAwesome::PushSolid();
        auto iconUtf8 = FontAwesome::UnicodeToUtf8(0xf0eb);

        ImGuiMCP::Text("Testing Menu");
        ImGuiMCP::PopStyleColor();

        if (ImGuiMCP::Checkbox("Enable Lights being disabled based on distance (not very usefull)", &globals::maxLightDistanceEnabled)) {
            ImGuiMCP::SetTooltip("disable all lights not within certain distance, does not reenable them currently)");
        }

        if (ImGuiMCP::IsItemHovered()) {
            ImGuiMCP::SetTooltip("Sets max distance for lights (does not re-enable them)");
        }

        if (globals::maxLightDistanceEnabled) {
            ImGuiMCP::InputInt("Global Max Light Distance", &globals::maxLightDistance);
        }

    

        if (ImGuiMCP::Checkbox("disable lights with radius that does not enter camera (and renables them)", &globals::disableLightsNotInCameraEnabled)) {
           
        }

        if (ImGuiMCP::IsItemHovered()) {
            ImGuiMCP::SetTooltip("was told vanilla does this already but im not sure it does when enabling light editor the log dump says otherwise)");
        }

        if (globals::disableLightsNotInCameraEnabled) {
            ImGuiMCP::SliderFloat("frustumTolerance", &globals::frustumOverlapTolerance, 0, 3);

            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip("how strict to disable lights outside the camera. higher = more strict");
            }
        
        }


        if (ImGuiMCP::Checkbox("enableHookToRemoveLightsFromBSTriShapes (Read tool tip)", &globals::enableHookToRemoveLightsFromBSTriShapes)) {
        }

        if (globals::enableHookToRemoveLightsFromBSTriShapes) {
            ImGuiMCP::SliderInt("min Light Overlap On TriShape mult", &globals::lightOverlapMinOnTriShapeMult, 0, 150);
            if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip(" 80 = light must cover 80% of the light");
        
        }

        if (ImGuiMCP::IsItemHovered()) {
            ImGuiMCP::SetTooltip("the formula is not compelte it needs work. trying other leads will come back to this");
        }

        if (ImGuiMCP::Button("Reinitialize Lights")) {

            auto player = RE::PlayerCharacter::GetSingleton(); 

            if (!player) return; 

            LightManager::reinitializeLightsWithinRange(player);  // your function
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

        ImGuiMCP::SliderInt("Logging Level", &globals::loggingLevel, 0, 3);
        if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("Logging Level (0: critical, 1: warnings/errors, 2: info)");

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
        name[0] = std::toupper(name[0]);
    }

    void __stdcall RenderLightEditor() {

        static int selectedIndex = -1;

        ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text, ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });

        FontAwesome::PushSolid();
        auto iconUtf8 = FontAwesome::UnicodeToUtf8(0xf044);

        ImGuiMCP::Text("%s Light Editor", iconUtf8.c_str());
        ImGuiMCP::PopStyleColor();
        ImGuiMCP::SameLine();

        float rowY = ImGuiMCP::GetCursorPosY();

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

                //                LightData::refillBankForSelectedTemplate(lightName, cfg);
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
                getAllLights();

            }
            else if (!enableLightEditor) {
                lights.clear();
            }
        }

        if (!enableLightEditor) {
            ImGuiMCP::Text("Light Editor is disabled. Enable it to edit light properties.");
            return;
        }

        if (ImGuiMCP::CollapsingHeader("Loaded Light Templates")) {

            // Count duplicates first
            std::unordered_map<std::string, int> nameCounts;
            std::unordered_map<std::string, int> nameIndex;
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
                    menuName = light->light->name.c_str();
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
                    menuName = light->light->name.c_str();
                }

                makeDisplayName(menuName);

                if (nameCounts[menuName] > 1) {
                    const int index = ++nameIndex[menuName];
                    menuName += " " + std::to_string(index);
                }

				ImGuiMCP::PushID(i);
                if (ImGuiMCP::Selectable(menuName.c_str(), &selected)) {
                    selectedIndex = i;
                }
				ImGuiMCP::PopID();
            }

            if (selectedIndex >= 0 && selectedIndex < lights.size()) {
                auto selectedLight = lights[selectedIndex];

                auto& lightData = selectedLight->light->GetLightRuntimeData();

                auto& dataExt = LightData::configIDToJsonCfg[lightData.unk138];

                auto* selectedIslRt = Overlay::Get(selectedLight->light.get());

                if (!selectedIslRt) {
                    logger::warn("no selected ISL runtime data in skse menu");
                    return;
                }


                if (!globals::islInstalled) {
                    if (ImGuiMCP::SliderFloat("Radius", &lightData.radius.x, 1.0f, 256.0f, "%.2f")) {
                        auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                        if (ssNode) {
                            auto& rt = ssNode->GetRuntimeData();
                            for (auto& light : rt.activeLights) {
                                if (!light) continue;
                                auto& existingRt = light->light->GetLightRuntimeData();
                                if (existingRt.unk138 != lightData.unk138) continue;
                                existingRt.radius = lightData.radius;
                            }
                            for (auto& light : rt.activeShadowLights) {
                                if (!light) continue;
                                auto& existingRt = light->light->GetLightRuntimeData();
                                if (existingRt.unk138 != lightData.unk138) continue;
                                existingRt.radius = lightData.radius;
                            }
                        }
                    }
                }

                // Fade
                if (ImGuiMCP::SliderFloat("Fade", &dataExt.startingFade, 0.0f, 10.0f, "%.1f")) {
                    auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                    if (ssNode) {
                        auto& rt = ssNode->GetRuntimeData();
                        for (auto& light : rt.activeLights) {
                            if (!light) continue;
                            auto& existingRt = light->light->GetLightRuntimeData();
                            if (existingRt.unk138 != lightData.unk138) continue;
                            existingRt.fade = dataExt.startingFade;
                        }
                        for (auto& light : rt.activeShadowLights) {
                            if (!light) continue;
                            auto& existingRt = light->light->GetLightRuntimeData();
                            if (existingRt.unk138 != lightData.unk138) continue;
                            existingRt.fade = dataExt.startingFade;
                        }
                    }
                }

                // Position
                if (ImGuiMCP::SliderFloat3("Position", &selectedLight->light->local.translate.x, -250.0f, 250.0f, "%.3f")) {
                    auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                    if (ssNode) {
                        auto& rt = ssNode->GetRuntimeData();
                        for (auto& light : rt.activeLights) {
                            if (!light) continue;
                            auto& existingRt = light->light->GetLightRuntimeData();
                            if (existingRt.unk138 != lightData.unk138) continue;
                            light->light->local.translate = selectedLight->light->local.translate;
                            if (auto* parent = light->light->parent) {
                                RE::NiUpdateData updateData{};
                                updateData.time = 0.0f;
                                updateData.flags = RE::NiUpdateData::Flag::kDirty;
                                parent->UpdateTransformAndBounds(updateData);
                            }
                        }
                        for (auto& light : rt.activeShadowLights) {
                            if (!light) continue;
                            auto& existingRt = light->light->GetLightRuntimeData();
                            if (existingRt.unk138 != lightData.unk138) continue;
                            light->light->local.translate = selectedLight->light->local.translate;
                            if (auto* parent = light->light->parent) {
                                RE::NiUpdateData updateData{};
                                updateData.time = 0.0f;
                                updateData.flags = RE::NiUpdateData::Flag::kDirty;
                                parent->UpdateTransformAndBounds(updateData);
                            }
                        }
                    }
                }

                // Flicker Intensity
                if (ImGuiMCP::SliderFloat("Flicker Intensity", &dataExt.flickerIntensity,
                    0.0f, 1.0f, "%.2f"))
                {
                }

                if (ImGuiMCP::SliderFloat("Flickers / Second", &dataExt.flickersPerSecond,
                    0.1f, 5.0f, "%.2f"))
                {

                }

                // RGB
                if (ImGuiMCP::SliderFloat3("RGB", &lightData.diffuse.red, 0.0f, 1.0f, "%.3f")) {
                    auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                    if (ssNode) {
                        auto& rt = ssNode->GetRuntimeData();
                        for (auto& light : rt.activeLights) {
                            if (!light) continue;
                            auto& existingRt = light->light->GetLightRuntimeData();
                            if (existingRt.unk138 != lightData.unk138) continue;
                            existingRt.diffuse = lightData.diffuse;
                        }
                        for (auto& light : rt.activeShadowLights) {
                            if (!light) continue;
                            auto& existingRt = light->light->GetLightRuntimeData();
                            if (existingRt.unk138 != lightData.unk138) continue;
                            existingRt.diffuse = lightData.diffuse;
                        }
                    }
                }

                // ISL sliders
                if (globals::islInstalled) {
                    // Cutoff
                    if (ImGuiMCP::SliderFloat("Cutoff (ISL)", &selectedIslRt->cutoffOverride, 0.01f, 0.99f, "%.2f")) {
                        auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                        if (ssNode) {
                            auto& rt = ssNode->GetRuntimeData();
                            for (auto& light : rt.activeLights) {
                                if (!light) continue;
                                auto& existingRt = light->light->GetLightRuntimeData();
                                if (existingRt.unk138 != lightData.unk138) continue;
                                if (auto* islRt = Overlay::Get(light->light.get())) {
                                    islRt->cutoffOverride = selectedIslRt->cutoffOverride;
                                }
                            }
                            for (auto& light : rt.activeShadowLights) {
                                if (!light) continue;
                                auto& existingRt = light->light->GetLightRuntimeData();
                                if (existingRt.unk138 != lightData.unk138) continue;
                                if (auto* islRt = Overlay::Get(light->light.get())) {
                                    islRt->cutoffOverride = selectedIslRt->cutoffOverride;
                                }
                            }
                        }
                    }

                    // Size
                    if (ImGuiMCP::SliderFloat("Size (ISL)", &selectedIslRt->size, 0.0f, 10.0f, "%.2f")) {
                        auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                        if (ssNode) {
                            auto& rt = ssNode->GetRuntimeData();
                            for (auto& light : rt.activeLights) {
                                if (!light) continue;
                                auto& existingRt = light->light->GetLightRuntimeData();
                                if (existingRt.unk138 != lightData.unk138) continue;
                                if (auto* islRt = Overlay::Get(light->light.get())) {
                                    islRt->size = selectedIslRt->size;
                                }
                            }
                            for (auto& light : rt.activeShadowLights) {
                                if (!light) continue;
                                auto& existingRt = light->light->GetLightRuntimeData();
                                if (existingRt.unk138 != lightData.unk138) continue;
                                if (auto* islRt = Overlay::Get(light->light.get())) {
                                    islRt->size = selectedIslRt->size;
                                }
                            }
                        }
                    }
                }
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
            auto lightName = light->light->name.c_str();

            if (!lightName || lightName[0] != 'R' || lightName[1] != 'L')
                continue;

            auto& currentRt = light->light->GetLightRuntimeData();

            auto& dataExt = LightData::configIDToJsonCfg[currentRt.unk138];

            // I use the enable light editor button this func is attached to as a debugger to check if light values get messed up
            // by flicker equation (they were before Its good to check sometimes)
            logger::debug("light :{}  fade:{}  starting fade:{}, radius: {}, flickerIntensity: {}, FlickerPerSecond{}, BSLight World Pos: {}, NiLight World Pos{} configID: {} ",
                lightName, currentRt.fade, dataExt.startingFade, currentRt.radius, dataExt.flickerIntensity, dataExt.flickersPerSecond, light->worldTranslate, light->light->world.translate, dataExt.configID);

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
            auto lightName = shadowLight->light->name.c_str();

            if (!lightName || lightName[0] != 'R' || lightName[1] != 'L')
                continue;

            auto& currentRt = shadowLight->light->GetLightRuntimeData();

            auto& dataExt = LightData::configIDToJsonCfg[currentRt.unk138];

            logger::debug("shadow light :{}  fade:{}  starting fade:{}, radius: {}, flickerIntensity: {}, FlickerPerSecond{},BSLight World Pos: {}, NiLight World Position {}, configID: {} ",
                lightName, currentRt.fade, dataExt.startingFade, currentRt.radius,
                dataExt.flickerIntensity, dataExt.flickersPerSecond, shadowLight->worldTranslate, shadowLight->light->world.translate, dataExt.configID);

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

        auto itDataExt = LightData::configIDToJsonCfg.find(lightData.unk138);
        if (itDataExt == LightData::configIDToJsonCfg.end()) {
            logger::warn("No JSON config entry found for light ID {}", lightData.unk138);
            return;
        }
        auto& dataExt = itDataExt->second;

        lightData.radius = LightData::getNiPointLightRadius(backupCfg);
        lightData.fade = backupCfg.fade;
        LightData::setNiPointLightAmbientAndDiffuse(light.get(), backupCfg);
        LightData::setNiPointLightPos(light.get(), backupCfg);

        dataExt.startingFade = backupCfg.startingFade;
        dataExt.flickerIntensity = backupCfg.flickerIntensity;
        dataExt.flickersPerSecond = backupCfg.flickersPerSecond;

        // Propagate to active lights in the shader node
        auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
        if (!ssNode) return;

        auto& rt = ssNode->GetRuntimeData();

        auto updateLightList = [&](auto& lightList) {
            for (auto& currentLight : lightList) {
                if (!currentLight) continue;

                auto& activeData = currentLight->light->GetLightRuntimeData();

                if (activeData.unk138 == lightData.unk138)
                    activeData = lightData;
                currentLight->light->local.translate = light->local.translate;

                if (!globals::islInstalled) continue;

                if (auto* isl = Overlay::Get(currentLight->light.get())) {
                    isl->cutoffOverride = backupCfg.cutoffOverride;
                    isl->size = backupCfg.size;
                }
            }
        };

        updateLightList(rt.activeLights);
        updateLightList(rt.activeShadowLights);

        logger::info("Restored '{}' to default config", lightName);
    }
}