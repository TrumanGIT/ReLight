#include "pages/settings_page.h"
#include "../global.h"
#include "../ticker.h"
#include "../ini.h"
#include "../folders.h"

using namespace UI;


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

    ImGuiMCP::ImVec2 rectMax = ImGuiMCP::GetItemRectMax();
    ImGuiMCP::ImVec2 rectMin = ImGuiMCP::GetItemRectMin();
    ImGuiMCP::ImVec2 winPos = ImGuiMCP::GetWindowPos();

    float iconX = (rectMax.x - winPos.x) + 10.0f;
    float iconY = (rectMin.y - winPos.y) + 4.0f;

    if (saveINIClicked) {
        bool ok = false;
        saveINIButton.set(buttonState::Working);
        ok = ini::saveSettingsToIni();
        saveINIButton.set(ok ? buttonState::Success : buttonState::Fail, 2.0f);
    }

    renderDone(saveINIButton, iconX, iconY);

    ImGuiMCP::SameLine();

    if (ImGuiMCP::Button("Debug log all lights")) {
        debugLogAllLights();
    }

    if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("Log all currently active relight lights to relight.log file");

    ImGuiMCP::Separator();

    ImGuiMCP::Checkbox("Disable Game Lights", &globals::disableGameLights);
    if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("Disable all game lights except for those in exclude by light editor ID section in Relight.ini\n Used so the Relight Official light add on can start with a clean base");

    ImGuiMCP::Checkbox("Remove Fake Glow Orbs", &globals::removeFakeGlowOrbs);
    if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("Remove fake glow orbs used by Bethesda");

    ImGuiMCP::Checkbox("All Relights As ISL", &globals::allRelightsAsISL);
    if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("Make all Relights have Inverse Squared Lighting regardless of ISL flag fake glow orbs used by Bethesda, Requires Cell Rrset");

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
    if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("Any light futher then this value will not draw debug lines");


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

                    // skip vanilla lights
                    if (cfg.isPluginLight) continue;

                    auto ref = light->light->GetUserData();

                    if (ref) {
                        lightRt.fade =
                            cfg.brightness *
                            ref->GetScale() *
                            globals::brightnessModifier *
                            Folders::Get(cfg.folder).brightness;
                    }
                    else {
                        lightRt.fade =
                            cfg.brightness *
                            globals::brightnessModifier *
                            Folders::Get(cfg.folder).brightness;
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

    if (ImGuiMCP::IsItemHovered()) {
        ImGuiMCP::BeginTooltip();
        ImGuiMCP::Text("This only works on cell reset");
        ImGuiMCP::EndTooltip();
    }

    Folders::RenderMenu();

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