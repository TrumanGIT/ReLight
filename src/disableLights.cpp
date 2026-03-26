
#include "disableLights.h"
#include "Utility.h"
#include "LightManager.h"


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

    toLower(modName); 

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

//what I do here is reimplement this func, and collect closest lights once 1 second after cell loaded (otherwise no lights yet)
// then I store the array index in a shader propertys unused forced darkness member field for fast lookups after that

//meh321 - intellightent
bool BSLightingShaderProperty_IsLightAffectingSurface::thunk(
    RE::BSLightingShaderProperty* p,
    RE::BSLight* light)
{
    if (!p || !light || !light->light) return false;

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
    // return here or crash on effect shaders 
    if (p->GetMaterialType() == RE::BSShaderMaterial::Type::kEffect) return true;

    auto ui = RE::UI::GetSingleton();
    if (ui && (ui->GameIsPaused() || ui->IsMenuOpen("CraftingMenu"))) return true;

    if (!globals::secondAfterCellFullyLoaded.load() || !globals::enableLightFlickerPreventionMeasures) return true;

    // torhces
    if (light->unk060 == 4) return true;

    auto pass = p->renderPassList.head;
    if (!pass || !pass->geometry) return true;
    
    // dont want to block large tri shapes that might supposed have more then 7 lights (windhelmBridge) 
    if (pass->geometry->worldBound.radius > 1800) return true;

    // probobly a sky light or something we should return
    if (light->light->radius.x > 1000) return true; 

    //return on actors 
    auto objectRef = p->renderPassList.head->geometry->GetUserData(); 

    if (objectRef) {
        if (objectRef->IsActor()) return true;
    }

    std::lock_guard lock(LightData::triLightCacheMutex);
    uint16_t currentGen = LightData::triLightCacheGeneration.load();
    uint16_t storedGen = 0, storedIdx = 0;
    UnpackFD(p->forcedDarkness, storedGen, storedIdx);

    // forced darkness is key to light list, if player switches cells, the pool of cached light lists changes
    if (p->forcedDarkness == 0.0f || storedGen != currentGen) {
        LightData::TriLightCache entry{};
        entry.lightShaderProp = p;
        LightManager::ComputeClosestLights(entry.lights, p);
        uint16_t newIdx = static_cast<uint16_t>(LightData::triLightCache.size() + 1);
        LightData::triLightCache.push_back(entry);
        p->forcedDarkness = PackFD(currentGen, newIdx);
        storedIdx = newIdx;
    }

    if (storedIdx == 0 || storedIdx > static_cast<uint16_t>(LightData::triLightCache.size()))
        return true;

    auto& cache = LightData::triLightCache[storedIdx - 1];
    for (int i = 0; i < 7; i++) {
        if (cache.lights[i] == light)
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

