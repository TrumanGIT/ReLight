#include "Menu.h"
#include "global.h"
#include "Functions.h"

namespace logger = SKSE::log;


//TODO:: add sliders for flicker settings and mabye light attenuation if its beneficial

namespace UI {

    static vector<RE::NiPointer<RE::BSLight>> lights = {};
    static bool lightsLoaded = false;
    static bool enableLightEditor = false;
    static bool lightAlreadyInList = false;

    void Register() {
        if (!SKSEMenuFramework::IsInstalled()) return;

        SKSEMenuFramework::SetSection("ReLight");

        SKSEMenuFramework::AddSectionItem("Settings", UI::RenderSettings);

        SKSEMenuFramework::AddSectionItem("Light Editor", UI::RenderLightEditor);
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

        ImGuiMCP::Checkbox("Disable Shadow Casters", (bool*)&disableShadowCasters);
        if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("Remove shadow-casting from lights");

        ImGuiMCP::Checkbox("Disable Torch Lights", &disableTorchLights);
        if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("Turn off all torch-type lights");

        ImGuiMCP::Checkbox("Remove Fake Glow Orbs", &removeFakeGlowOrbs);
        if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("Remove fake glow orbs used by Bethesda");

        ImGuiMCP::Separator();

        ImGuiMCP::SliderInt("Logging Level", &loggingLevel, 0, 3);
        if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("Logging Level (0: critical, 1: warnings/errors, 2: info)");

        ImGuiMCP::Separator();

        if (ImGuiMCP::CollapsingHeader("Whitelist (by plugin name)")) {
            for (auto& entry : whitelist)
                ImGuiMCP::Text("%s", entry.c_str());
        }

        if (ImGuiMCP::CollapsingHeader("Priority Nodes")) {
            for (auto& entry : priorityList)
                ImGuiMCP::Text("%s", entry.c_str());
        }

        if (ImGuiMCP::CollapsingHeader("Excluded Nodes (Exact)")) {
            for (auto& entry : exclusionList)
                ImGuiMCP::Text("%s", entry.c_str());
        }

        if (ImGuiMCP::CollapsingHeader("Excluded Nodes (Partial Match)")) {
            for (auto& entry : exclusionListPartialMatch)
                ImGuiMCP::Text("%s", entry.c_str());
        }
    }

    void __stdcall RenderLightEditor() {

        static int selectedIndex = -1;

        ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text, ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });

        FontAwesome::PushSolid();
        auto iconUtf8 = FontAwesome::UnicodeToUtf8(0xf044);

        ImGuiMCP::Text("%s Light Editor", iconUtf8.c_str());
        ImGuiMCP::PopStyleColor();
        ImGuiMCP::SameLine();

        if (ImGuiMCP::Button("Save")) {

            if (selectedIndex >= 0 && selectedIndex < lights.size()) {
                auto selectedLight = lights[selectedIndex];
                auto niLight = selectedLight->light.get();
                if (!niLight) {
                    logger::error("no ni light from bslight when saving template");
                    return;
                }

                std::string lightName = niLight->name.c_str();
                if (lightName.empty()) {
                    logger::debug("Nilight name empty when saving template");
                    return;
                }

                LightConfig cfg;

                if (LightData::findConfigForLight(cfg, lightName)) {
                    LightData::updateConfigFromLight(cfg, niLight);
                    if (!cfg.configPath.empty()) {
                        saveConfiguration(cfg, cfg.configPath);
                    }
                    else {
                        logger::warn("Config for '{}' has no configPath, cannot save", cfg.nodeName);
                    }
                }
                else {
                    logger::warn("No config found for light '{}'", lightName);
                }

//                LightData::refillBankForSelectedTemplate(lightName, cfg);

                logger::debug("refilled Bank for selectedTemplates (probobly)");
            }

        }

        if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("Save the currently selected light template's settings");

        ImGuiMCP::SameLine();

        if (ImGuiMCP::Button("Default")) {
            if (selectedIndex >= 0 && selectedIndex < lights.size()) {
                auto selectedLight = lights[selectedIndex];
             

                restoreLightToDefaults(selectedLight->light);

                logger::info("Restored defaults for '{}'", selectedLight->light->name.c_str());
            }
        }

        if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("Restore the currently selected light template's settings to what they were at game start");

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

            for (int i = 0; i < lights.size(); i++) {

                auto& light = lights[i];
                if (!light) continue;

                bool selected = (i == selectedIndex);
                if (ImGuiMCP::Selectable(light->light->name.c_str(), &selected)) {
                    selectedIndex = i;
                }
            }


            if (selectedIndex >= 0 && selectedIndex < lights.size()) {
                auto selectedLight = lights[selectedIndex];

                auto& lightData = selectedLight->light->GetLightRuntimeData();

                auto* selectedIslRt = ISL_Overlay::Get(selectedLight->light.get());

                if (!selectedIslRt) return;

                //TODO:: add 'starting radius' so it doesent fight skse menu when ISL is enabled

                if (ImGuiMCP::SliderFloat("Radius", &lightData.radius.x, 1.0f, 256.0f, "%.2f")) {
                    auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                    if (ssNode) {
                        auto& rt = ssNode->GetRuntimeData();
                        for (auto& light : rt.activeLights) {
                            if (light && light->light->name == selectedLight->light->name) {
                                light->light->GetLightRuntimeData().radius = lightData.radius;
                            }
                        }
                    }
                }

                if (ImGuiMCP::SliderFloat("Fade", &selectedIslRt->startingFade, 0.0f, 10.0f, "%.1f")) {
           
                    auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                    if (ssNode) {
                        auto& rt = ssNode->GetRuntimeData();
                        for (auto& light : rt.activeLights) {
                            if (light && light->light->name == selectedLight->light->name) {
                                auto& data = light->light->GetLightRuntimeData();
                                data.fade = selectedIslRt->startingFade;
                            }
                        }
                    }
                }


                //static float color[3] = { lightData.diffuse.red, lightData.diffuse.green, lightData.diffuse.blue };
                if (ImGuiMCP::SliderFloat3("RGB", &lightData.diffuse.red, 0.000f, 1.000f, "%.3f")) {
                    auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                    if (ssNode) {
                        auto& rt = ssNode->GetRuntimeData();
                        for (auto& light : rt.activeLights) {
                            if (light && light->light->name == selectedLight->light->name) {
                                light->light->GetLightRuntimeData().diffuse = lightData.diffuse;
                            }
                        }
                    }
                }

               

                if (ImGuiMCP::SliderFloat("Cutoff (ISL)", &selectedIslRt->cutoffOverride, 0.01f, 0.99f, "%.2f")) {
                    auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                    if (ssNode) {
                        auto& rt = ssNode->GetRuntimeData();
                        for (auto& light : rt.activeLights) {
                            if (light && light->light->name == selectedLight->light->name) {
                                if (auto* islRt = ISL_Overlay::Get(light->light.get())) {
                                    islRt->cutoffOverride = selectedIslRt->cutoffOverride;
                                }
                            }
                        }
                    }
                }

                if (ImGuiMCP::SliderFloat("Size (ISL)", &selectedIslRt->size, 0.00f, 10.00f, "%.2f")) {
                    auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                    if (ssNode) {
                        auto& rt = ssNode->GetRuntimeData();
                        for (auto& light : rt.activeLights) {
                            if (light && light->light->name == selectedLight->light->name) {
                                if (auto* islRt = ISL_Overlay::Get(light->light.get())) {
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
        outFile << "loggingLevel=" << loggingLevel << "\n\n";

        outFile << "; disable all shadow - casting light references(except skylights) (default = true)\n";
        outFile << "disableShadowCasters=" << (disableShadowCasters ? "true" : "false") << "\n\n";

        outFile << "; disable light references for carryable torches(default = true)\n";
        outFile << "disableTorchLights=" << (disableTorchLights ? "true" : "false") << "\n\n";

        outFile << "; remove fake glow orbs (default = true)\n";
        outFile << "removeFakeGlowOrbs=" << (removeFakeGlowOrbs ? "true" : "false") << "\n\n";

        outFile << "; add esps by name to undisable their lights (usually not needed)\n";
        outFile << "whitelist=";
        for (size_t i = 0; i < whitelist.size(); i++) {
            outFile << whitelist[i];
            if (i + 1 < whitelist.size()) outFile << ",";
        }
        outFile << "\n\n";

        outFile << "; exclude specific nodes\n";
        for (auto& node : exclusionList)
            outFile << node << "\n";

        outFile << "\n; exclude partial nodes\n";
        for (auto& node : exclusionListPartialMatch)
            outFile << node << "\n";

        outFile << "\n; priority list (higher = first match. Usefull for candlechandelier ect to get correct lighting)\n";
        for (auto& node : priorityList)
            outFile << node << "\n";

        outFile.close();
        logger::info("ReLight.ini saved successfully!");
    }

    void getAllLights() {
        auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
        if (!ssNode) {
            logger::warn("ShadowSceneNode[0] is null!");
            return;
        }

        auto& rt = ssNode->GetRuntimeData();

        for (auto& light : rt.activeLights) {
            if (!light) continue; 
            auto lightName = light->light->name;

            for (auto& existingLight : lights) {
                if (existingLight->light->name == lightName) {
                    // Light already exists in the list, skip adding
                    lightAlreadyInList = true;
                }
            }

            if (!lightAlreadyInList) lights.push_back(light);

            lightAlreadyInList = false;
        }
    }

    void restoreLightToDefaults(RE::NiPointer<RE::NiLight> selectedLight) {
        if (!selectedLight) {
            logger::warn("Selected light is null, cannot restore defaults");
            return;
        }

        const std::string lightName = selectedLight->name.c_str();
        auto defaultIt = LightData::defaultConfigs.find(RemoveSuffix(lightName, "RL"));
        if (defaultIt == LightData::defaultConfigs.end()) {
            logger::warn("No default config found for '{}'", lightName);
            return;
        }

        const auto& defaultCfg = defaultIt->second;

        // Update selected light runtime data
        auto& lightData = selectedLight->GetLightRuntimeData();
        lightData.radius = LightData::getNiPointLightRadius(defaultCfg);
        lightData.fade = defaultCfg.fade;
        lightData.diffuse.red = defaultCfg.diffuseColor[0] / 255.0f;
        lightData.diffuse.green = defaultCfg.diffuseColor[1] / 255.0f;
        lightData.diffuse.blue = defaultCfg.diffuseColor[2] / 255.0f;

        // Propagate to active lights in the shader node
        auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
        if (ssNode) {
            auto& rt = ssNode->GetRuntimeData();
            for (auto& light : rt.activeLights) {
                if (!light || light->light->name.c_str() != lightName)
                    continue;

                auto& activeData = light->light->GetLightRuntimeData();
                activeData = lightData;

                if (auto* isl = ISL_Overlay::Get(light->light.get())) {
                    isl->cutoffOverride = defaultCfg.cutoffOverride;
                    isl->size = defaultCfg.size;
                }
            }
        }

        logger::info("Restored '{}' to default config", lightName);
    }

}