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
    if (LightManager::pendingMerges.empty()) return;

    auto now = std::chrono::steady_clock::now();
    std::lock_guard lock(LightManager::pendingMergesMutex);

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

                    if (HasAnythingBetween(refA.get(), refB.get())) {
                        logger::debug("obstacle detected between refA {:08X} and refB {:08X}, giving refB its own light",
                            refA->GetFormID(), refB->GetFormID());

                        //globals::mergedRefs.erase(refB->GetFormID());
                        auto refB3D = refB->Get3D();
                        if (!refB3D) continue;

                        auto a_root = refB3D->AsNode();
                        if (!a_root) {
                            logger::warn("no ni node casted from niav object in pending merges");
                            continue;
                        }

                        const RE::BSFixedString nodeNameMatch = findPriorityMatch(a_root->name);
                        if (nodeNameMatch.empty()) continue;
                        if (isExclude(a_root->name, refB->GetFormID())) continue;

                        std::string matchStr = nodeNameMatch.c_str();
                        auto refFormID = refB->GetFormID();

                        auto ui = RE::UI::GetSingleton();
                        if (ui && ui->IsMenuOpen("InventoryMenu")) continue;

                        const auto baseObject = refB->GetBaseObject();
                        const auto baseFormID = baseObject ? baseObject->GetFormID() : 0;
                        if (baseFormID != 0) {
                            globals::baseFormsWithAttachedLights.emplace(baseFormID);
                            logger::debug("processing ref {:08X} with node name: {} with baseFormID: {} emplaced in set", refFormID, matchStr, baseFormID);
                        }

                        if (globals::removeFakeGlowOrbs)
                            glowOrbRemover(a_root);

                        auto cell = refB->GetParentCell();
                        if (!cell) {
                            logger::warn("no cell cant determine if should use exterior or interior configs");
                            continue;
                        }

                        bool isInterior = cell->IsInteriorCell();
                        auto cfgs = findConfigsForNode(matchStr, isInterior);

                        for (auto& cfg : cfgs) {
                            auto cloneLight = cloneNiPointLight(LightData::masterNiPointLight.light.get());
                            if (!cloneLight) {
                                logger::warn("Failed to clone NiPointLight for node '{}' for ref {:08X})", matchStr, refFormID);
                                continue;
                            }

                            LightManager::attachLightUsingAttachPath(cfg, a_root, cloneLight, refFormID);
                            LightData::setNiPointLightDataFromCfg(cloneLight, cfg);
                            cloneLight->name = "RL" + matchStr;
                            LightManager::attachNiPointLightToShadowSceneNode(cloneLight, cfg, refB.get());
                        }

                        continue; // don't add to validCandidates
                    }

                    validCandidates.push_back(handle);
                }

                LightManager::finalizeMerge(entry, validCandidates);
                return true;
            }),
        LightManager::pendingMerges.end());
}

/*inline void OnFrameUpdate() {
    auto now = std::chrono::steady_clock::now();
	if (LightManager::pendingMerges.empty()) return;
    std::lock_guard lock(PendingProperty::mutex);

	PendingProperty::list.erase(
        std::remove_if(PendingProperty::list.begin(), PendingProperty::list.end(),
            [&](PendingProperty& entry) {
                if (!entry.prop) return true; 

                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    now - entry.registeredAt).count();

                if (elapsed < 1) return false; // not ready yet, keep in list

				auto pass = entry.prop->renderPassList.head;
				if (!pass || !pass->geometry) return false;

				uint32_t count = pass->numLights;

					if (count == 0) {
						entry.prop->forcedDarkness = 1.0f; // permanently skip, will always return true in hook
						return true; // remove from list
					}

					if (pass->geometry->worldBound.radius > globals::maxWallSizeForStrictLightBounds) count++; 

                    entry.prop->forcedDarkness = static_cast<float>(count + 1);
                    logger::debug("[LightHook] Cached after delay: {}", count + 1);
                //}
                return true; // done, remove
            }),
		PendingProperty::list.end());
}*/