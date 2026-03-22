#pragma once

#include "logger.hpp"
#include "random.h"
#include "LightData.h"
#include "disableLights.h"
#include "LightManager.h"
#include "utility.h"

// hook into player update so we can update light flicker data every fram
struct PlayerCharacter_Update {

    static void thunk(RE::PlayerCharacter* player, float delta);

    static inline REL::Relocation<decltype(thunk)> func;

    static void Install();
};


// used in flicker calcs
inline float getRandomFloat(const float& min, const float& max, uint32_t rngState)
{
	return min + (max - min) * Random::rand(rngState);
}

// generic type argument probly not needed both shadow light list and non shadow light list same array type proboblly
template <class T>
static void ApplyLightFlicker(T& lights, float delta)
{
	for (auto& light : lights) {
		if (!light)
			continue;

		const char* name = light->light->name.c_str();
		if (!name || name[0] != 'R' || name[1] != 'L')
			continue;

		// free float used as flicker timer
		auto& scale = light->light->local.scale;
		auto& rt = light->light->GetLightRuntimeData();

		auto it = LightData::configIDToJsonCfg.find(rt.unk138);
		if (it == LightData::configIDToJsonCfg.end())
			continue;

		const auto& dataExt = it->second;

		uint32_t seed =
			static_cast<uint32_t>(
				reinterpret_cast<std::uintptr_t>(light->light.get()) & 0xFFFFFFFF);

		const float r = getRandomFloat(-0.1f, 0.1f, seed);

		scale += delta * (1.0f - r) * std::numbers::pi_v<float>;
		rt.fade =
			dataExt.startingFade +
			std::sin(scale * dataExt.flickersPerSecond) * dataExt.flickerIntensity;
	}
}

inline void handlePendingMerges() {
    std::lock_guard lock(LightManager::pendingMergesMutex);
    if (LightManager::pendingMerges.empty()) return;

    auto now = std::chrono::steady_clock::now();

    std::vector<RE::ObjectRefHandle> reprocessQueue{};

    LightManager::pendingMerges.erase(
        std::remove_if(LightManager::pendingMerges.begin(), LightManager::pendingMerges.end(),
            [&](LightManager::PendingMerge& entry) {

                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - entry.registeredAt).count();
                if (elapsed < 250) return false;

                auto refA = entry.refA.get();
                if (!refA) return true;

                std::vector<RE::ObjectRefHandle> validCandidates;

                for (const auto& handle : entry.candidateHandles) {
                    auto refB = handle.get();
                    if (!refB) continue;

                    // ray cast to ensure no walls in between
                    if (HasAnythingBetween(refA.get(), refB.get())) {
                        reprocessQueue.emplace_back(handle);
                        continue;
                    }
              
                    validCandidates.push_back(handle);
                }

                LightManager::finalizeMerge(entry, validCandidates);
                return true;
            }),
        LightManager::pendingMerges.end());

    while (!reprocessQueue.empty()) {

        auto retryRefAHandle = reprocessQueue.front();
        reprocessQueue.erase(reprocessQueue.begin());

        auto retryRefA = retryRefAHandle.get();
        if (!retryRefA) continue;

        std::vector<RE::ObjectRefHandle> nextQueue;
        std::vector<RE::ObjectRefHandle> mergeGroup;

        for (const auto& handle : reprocessQueue) {

            auto otherRef = handle.get();
            if (!otherRef) continue;

            auto formID = otherRef->GetFormID();
          
            if (HasAnythingBetween(retryRefA.get(), otherRef.get())) {
                nextQueue.emplace_back(handle);
            }
            else {
                mergeGroup.emplace_back(handle);
            }
        }

        auto root = retryRefA->Get3D();
        if (!root) { reprocessQueue = std::move(nextQueue); continue; }

        auto rootNode = root->AsNode();
        if (!rootNode) { reprocessQueue = std::move(nextQueue); continue; }

        auto base = retryRefA->GetBaseObject();
        auto model = base ? base->As<RE::TESModel>() : nullptr;
        if (!model) { reprocessQueue = std::move(nextQueue); continue; }

        std::string meshName = extractMeshName(model->GetModel());
        std::string match = std::string(findPriorityMatch(meshName));
        if (match.empty()) { reprocessQueue = std::move(nextQueue); continue; }

        auto cell = retryRefA->GetParentCell();
        if (!cell) { reprocessQueue = std::move(nextQueue); continue; }

        auto cfgs = findConfigsForNode(match, cell->IsInteriorCell());
        if (cfgs.empty()) {
            logger::warn("Dropping ref {:08X} — no configs found", retryRefA->GetFormID());
            // keep all other refs for retry
            nextQueue.insert(nextQueue.end(), mergeGroup.begin(), mergeGroup.end());
            reprocessQueue = std::move(nextQueue);
            continue;
        }

        uint32_t flags = cfgs[0].flags;
  
        if (cfgs.size() == 1 && !(flags & static_cast<uint32_t>(LIGHT_FLAGS::kNoMerging))) {
            auto cloneLight = cloneNiPointLight(LightData::masterNiPointLight.light.get());
            if (!cloneLight) { reprocessQueue = std::move(nextQueue); continue; }

            logger::debug("RE processing ref {:08X} with light {}", retryRefA->GetFormID(), match);

            LightManager::attachLightUsingAttachPath(cfgs[0], rootNode, cloneLight, retryRefA->GetFormID());

            LightManager::PendingMerge p;
            p.refA = retryRefAHandle;
            p.refARoot = rootNode;
            p.candidateHandles = mergeGroup;
            p.light = RE::NiPointer<RE::NiPointLight>(cloneLight);
            p.refALightName = match;
            p.winningConfig = cfgs[0]; 

            LightManager::finalizeMerge(p, mergeGroup);
        }
        //TODO:: doesent attach debug markers
        else {
            for (const auto& cfg : cfgs) {
                auto cloneLight = cloneNiPointLight(LightData::masterNiPointLight.light.get());
                if (!cloneLight) continue;

                LightManager::attachLightUsingAttachPath(cfg, rootNode, cloneLight, retryRefA->GetFormID());
                LightData::setNiPointLightDataFromCfg(cloneLight, cfg);
                LightManager::attachNiPointLightToShadowSceneNode(cloneLight, cfg, retryRefA.get());
            }

            // multi-light can't merge, retry others
            nextQueue.insert(nextQueue.end(), mergeGroup.begin(), mergeGroup.end());
        }

        reprocessQueue = std::move(nextQueue);
    }
}

inline bool OneSecondPassed(const std::chrono::steady_clock::time_point& timerStart)
{

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - timerStart).count();

    return elapsed >= 1;
}
