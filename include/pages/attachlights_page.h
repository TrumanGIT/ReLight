#pragma once

#include "../SKSEMenuFramework.h"
#include "../logger.hpp"
#include "../global.h"

void __stdcall RenderAttachRemove();


inline bool RenderYellowButton(const char* a_label)
{
    ImGuiMCP::PushStyleVar(ImGuiMCP::ImGuiStyleVar_FrameRounding, 5.0F);
    ImGuiMCP::PushStyleVar(ImGuiMCP::ImGuiStyleVar_FrameBorderSize, 1.0F);

    ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Button, ImGuiMCP::ImVec4{ 0.60F, 0.50F, 0.10F, 0.80F });
    ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_ButtonHovered, ImGuiMCP::ImVec4{ 0.80F, 0.65F, 0.15F, 0.90F });
    ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_ButtonActive, ImGuiMCP::ImVec4{ 0.40F, 0.35F, 0.05F, 1.00F });

    ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text, ImGuiMCP::ImVec4{ 1.00F, 0.95F, 0.90F, 1.00F });
    ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Border, ImGuiMCP::ImVec4{ 0.80F, 0.65F, 0.15F, 0.60F });

    const bool clicked = ImGuiMCP::Button(a_label);

    ImGuiMCP::PopStyleColor(5);
    ImGuiMCP::PopStyleVar(2);

    return clicked;
}

inline bool RenderRedButton(const char* a_label)
{
    ImGuiMCP::PushStyleVar(ImGuiMCP::ImGuiStyleVar_FrameRounding, 5.0F);
    ImGuiMCP::PushStyleVar(ImGuiMCP::ImGuiStyleVar_FrameBorderSize, 1.0F);
    ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Button, ImGuiMCP::ImVec4{ 0.6F, 0.1F, 0.1F, 0.8F });
    ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_ButtonHovered, ImGuiMCP::ImVec4{ 0.8F, 0.2F, 0.2F, 0.9F });
    ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_ButtonActive, ImGuiMCP::ImVec4{ 0.4F, 0.05F, 0.05F, 1.0F });
    ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text, ImGuiMCP::ImVec4{ 1.0F, 0.9F, 0.9F, 1.0F });
    ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Border, ImGuiMCP::ImVec4{ 0.8F, 0.2F, 0.2F, 0.6F });

    const bool clicked = ImGuiMCP::Button(a_label);

    ImGuiMCP::PopStyleColor(5);
    ImGuiMCP::PopStyleVar(2);

    return clicked;
}

inline void RefreshNearbyObjectsByBase(RE::TESObjectREFR* selected, RE::FormID targetBaseFormID)
{
    if (!selected) {
        logger::error("no selected ref, cannot refresh nearby objects");
        return;
    }

    logger::debug("refresh lights called with base formID, {:08X}", targetBaseFormID);

    auto* player = RE::PlayerCharacter::GetSingleton();
    auto* tes = RE::TES::GetSingleton();

    if (!player || !tes) {
        logger::error("No Player or TES in refresh nearby objects, cant refresh");
        return;
    }

    tes->ForEachReferenceInRange(player, globals::fLODFadeOutMultObjects,
        [selected, targetBaseFormID](RE::TESObjectREFR* ref)
        {
            if (!ref || ref == selected) {
                return RE::BSContainer::ForEachResult::kContinue;
            }

            const auto base = ref->GetBaseObject();
            if (!base || base->GetFormID() != targetBaseFormID) {
                return RE::BSContainer::ForEachResult::kContinue;
            }

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
