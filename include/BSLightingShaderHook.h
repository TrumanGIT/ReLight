#pragma once

#include "logger.hpp"
#include "global.h"

struct BSLightingShader_SetupGeometry
{
    static void thunk(RE::BSShader* This, RE::BSRenderPass* Pass, uint32_t RenderFlags);
    static inline REL::Relocation<decltype(thunk)> func;

    static void Install();
};

struct BSShaderPropertyLightData_AttachLight
{
    static __int64 thunk(RE::BSShaderPropertyLightData* a_this, RE::BSLight* light);
    static inline std::uintptr_t func;
    static void Install();
};


struct BSLight_AddFadeNode
{
    static __int64 thunk(RE::BSLight* a_this, RE::NiAVObject* a_root);
    static inline std::uintptr_t func;
    static void Install();
};

struct BSLightingShaderProperty_IsLightAffectingSurface
{
    static bool thunk(RE::BSLightingShaderProperty* p, RE::BSLight* light);
    static inline REL::Relocation<decltype(thunk)> func;
    static void Install();
};


inline void BuildClosestCandles(
    RE::BSLightingShaderProperty* p,
    RE::BSTArray<RE::NiPointer<RE::BSLight>>& allLights,
    const RE::NiPoint3& triCenter
)  // hard cutoff in world units
{
    if (!p) return;

    std::vector<std::pair<float, RE::BSLight*>> distances;

    for (auto& light : allLights) {
        if (!light || light->unk060 != 2)  // candles only
            continue;

        auto* l = light.get();
        if (!l || !l->light)
            continue;

        const float dist = triCenter.GetDistance(l->light->world.translate);

        //  distance-only reject
        if (dist > globals::gMinCandleCoverage)
            continue;

        distances.emplace_back(dist, l);
    }

    // stable, deterministic ordering
    std::sort(distances.begin(), distances.end(),
        [](const auto& a, const auto& b) {
            return a.first < b.first;
        });

    std::vector<RE::BSLight*> closest;
    for (size_t i = 0; i < distances.size() && i < 6; ++i) {
        closest.push_back(distances[i].second);
    }

    globals::gTriClosestCandles[p] = std::move(closest);

    // mark built
    p->forcedDarkness = 1.0f;
}

// Check if the candle is one of the closest 5
inline bool IsCandleRelevant(RE::BSLightingShaderProperty* p, RE::BSLight* light)
{
    if (!p || !light) return false;

    auto it = globals::gTriClosestCandles.find(p);
    if (it == globals::gTriClosestCandles.end())
        return true; // fallback if somehow not built yet

    const auto& vec = it->second;
    return std::find(vec.begin(), vec.end(), light) != vec.end();
}