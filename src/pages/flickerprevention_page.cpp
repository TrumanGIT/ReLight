#include "pages/flickerprevention_page.h"
#include "../global.h"
#include "../ticker.h"
#include "../ini.h"

using namespace UI; 

void __stdcall RenderLightFlickerPreventionMenu() {

    if (ImGuiMCP::BeginChild("Light Flicker Prevention", ImGuiMCP::ImVec2(0, 380), true,
        ImGuiMCP::ImGuiWindowFlags_NoScrollbar))
    {
        ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text, ImGuiMCP::ImVec4{ 1.0f, 0.85f, 0.4f, 1.0f });

        ImGuiMCP::Text("Light Flicker Prevention");

        ImGuiMCP::PopStyleColor();

        ImGuiMCP::SameLine();

        bool saveINIClicked = ImGuiMCP::Button("Save INI");

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

        ImGuiMCP::Text("(This only works with relight overhauls that use relight flags)");

        ImGuiMCP::ImVec2 avail = ImGuiMCP::GetContentRegionAvail();

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