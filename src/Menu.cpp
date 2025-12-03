#include "Menu.h"
#include "global.h"
#include "Functions.h"

namespace logger = SKSE::log;

namespace UI {

    void Register() {
        if (!SKSEMenuFramework::IsInstalled()) return;

        SKSEMenuFramework::SetSection("ReLight");

        SKSEMenuFramework::AddSectionItem("Settings", UI::Render);

        reLightMenuWindow = SKSEMenuFramework::AddWindow(RenderWindow);
    }

    void __stdcall Render() {
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

        if (ImGuiMCP::CollapsingHeader("Whitelist (by plugin name)")) {
            for (auto& entry : whitelist)
                ImGuiMCP::BulletText("%s", entry.c_str());
        }

        if (ImGuiMCP::CollapsingHeader("Priority Nodes")) {
            for (auto& entry : priorityList)
                ImGuiMCP::BulletText("%s", entry.c_str());
        }

        if (ImGuiMCP::CollapsingHeader("Excluded Nodes (Exact)")) {
            for (auto& entry : exclusionList)
                ImGuiMCP::BulletText("%s", entry.c_str());
        }

        if (ImGuiMCP::CollapsingHeader("Excluded Nodes (Partial Match)")) {
            for (auto& entry : exclusionListPartialMatch)
                ImGuiMCP::BulletText("%s", entry.c_str());
        }
    }

    void __stdcall RenderWindow() {
        auto viewport = ImGuiMCP::GetMainViewport();
        ImGuiMCP::SetNextWindowSize(ImGuiMCP::ImVec2{ viewport->Size.x * 0.40f,
                                         viewport->Size.y * 0.50f },
            ImGuiMCP::ImGuiCond_Appearing);

        ImGuiMCP::Begin("ReLight Menu", nullptr, ImGuiMCP::ImGuiWindowFlags_NoTitleBar);
        Render();
        ImGuiMCP::End();
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
        outFile << "loggingLevel=2\n\n";

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

        outFile << "; Exclude specific nodes\n";
        for (auto& node : exclusionList)
            outFile << node << "\n";

        outFile << "\n; Exclude partial nodes\n";
        for (auto& node : exclusionListPartialMatch)
            outFile << node << "\n";

        outFile << "\n; Priority nodes\n";
        for (auto& node : priorityList)
            outFile << node << "\n";

        outFile.close();
        logger::info("ReLight.ini saved successfully!");
    }
}