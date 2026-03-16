
#include "disableLights.h"
#include "LightData.h"
#include "Utility.h"


//Po3's hook THIS DISABLES ALL LIGHTS TO START WITH A CLEAN BASE TO WORK FROM

RE::NiPointLight* TESObjectLIGH_GenDynamic::thunk(
	RE::TESObjectLIGH* light,
	RE::TESObjectREFR* ref,
	RE::NiNode* node,
	bool forceDynamic,
	bool useLightRadius,
	bool affectRequesterOnly)
{

	if (!ref || !light)
		return func(light, ref, node, forceDynamic, useLightRadius, affectRequesterOnly);

	if (shouldDisableLight(light, ref))
		return nullptr;

	return func(light, ref, node, forceDynamic, useLightRadius, affectRequesterOnly);
}

void TESObjectLIGH_GenDynamic::Install() {
	std::array targets{
		std::make_pair(RELOCATION_ID(17206, 17603), 0x1D3),  // TESObjectLIGH::Clone3D
		std::make_pair(RELOCATION_ID(19252, 19678), 0xB8),   // TESObjectREFR::AddLight
	};

	for (const auto& [address, offset] : targets) {
		REL::Relocation<std::uintptr_t> target{ address, offset };
		auto& trampoline = SKSE::GetTrampoline();
		TESObjectLIGH_GenDynamic::func = trampoline.write_call<5>(target.address(), TESObjectLIGH_GenDynamic::thunk);
	}

	logger::info("Installed TESObjectLIGH::GenDynamic patch");
}

bool TESObjectLIGH_GenDynamic::shouldDisableLight(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref)
{
	if (!ref || !light || ref->IsDynamicForm()) {
		return false;
	}

	if (LightData::excludeLightEditorID(light)) return false;

    const RE::TESFile* refOriginFile = ref->GetDescriptionOwnerFile();
    std::string modName = refOriginFile ? refOriginFile->fileName : "";

    for (const auto& whitelistedMod : globals::whitelist) {
        if (modName.find(whitelistedMod) != std::string::npos) {
            return false;
        }
    }

	auto player = RE::PlayerCharacter::GetSingleton();

	if (IsInSoulCairnOrApocrypha(player)) {
		logger::debug("player is in apocrypha or soul cairn so we should not disable light");
		return false;
	}

	return true;
}

//meh321 - intellightent
bool BSLightingShaderProperty_IsLightAffectingSurface::thunk(
    RE::BSLightingShaderProperty* p,
    RE::BSLight* light)
{
    if (!p || !light) return false;

    // vanilla logic
    if (!light->affectLand)
    {
        const auto& flags = p->flags;
        if (flags.any(RE::BSShaderProperty::EShaderPropertyFlag::kMultiTextureLandscape) ||
            flags.any(RE::BSShaderProperty::EShaderPropertyFlag::kNoLODLandBlend))
        {
            return false;
        }
    }
   
    if (!globals::cellFullyLoaded || !globals::enableLightFlickerPreventionMeasures) return true;

    if (p->forcedDarkness == 0.0f)
    {
        auto* pass = p->renderPassList.head;
        if (!pass || !pass->geometry)
            return true;

        LightData::TriLightCache entry{};
        entry.lightShaderProp = RE::NiPointer<RE::BSLightingShaderProperty>(p);

        ComputeClosestLights(entry.lights, p);

        {
            std::lock_guard lock(LightData::triLightCacheMutex);

            uint32_t index = static_cast<uint32_t>(LightData::triLightCache.size());
            LightData::triLightCache.push_back(entry);

            p->forcedDarkness = static_cast<float>(index + 1);
        }
    }

    auto pass = p->renderPassList.head;
    if (!pass || !pass->geometry) return false;

    float fd = p->forcedDarkness;
    if (fd < 1.0f)
        return true;

    std::lock_guard lock(LightData::triLightCacheMutex);

    if (fd > static_cast<float>(LightData::triLightCache.size()))
    {
        p->forcedDarkness = 0.0f;
        return true;
    }

    uint32_t idx = static_cast<uint32_t>(fd);
    auto& cache = LightData::triLightCache[idx - 1];

#pragma unroll
    for (int i = 0; i < 7; i++)
    {
        if (cache.lights[i].get() == light)
            return true;
    }
    return false;
}

//meh321 - intellightent
void BSLightingShaderProperty_IsLightAffectingSurface::Install()
{
    auto& trampoline = SKSE::GetTrampoline();

    REL::Relocation<std::uintptr_t> target{
        RELOCATION_ID(98902, 105550)
    };

    func = trampoline.write_branch<5>(
        target.address(),
        thunk
    );

    logger::info("BSLightingShaderProperty_IsLightAffectingSurface hook installed");
}

