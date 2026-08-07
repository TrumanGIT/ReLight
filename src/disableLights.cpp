
#include "disableLights.h"
#include "Utility.h"
#include "LightManager.h"
#include "forms.hpp"
#include "config.hpp"


//Po3's hook THIS DISABLES ALL LIGHTS TO START WITH A CLEAN BASE TO WORK FROM

//EDIT i now use this to also edit 
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

    std::string edid = clib_util::editorID::get_editorID(light);

    const RE::TESFile* refOriginFile = ref->GetDescriptionOwnerFile();
    std::string modName = refOriginFile ? refOriginFile->fileName : "";

    auto configs = LightManager::findConfigsForRef(ref, true);

    bool configExists = configs != nullptr && !configs->empty();

	if (!configExists && shouldDisableLight(light, ref, edid, modName))
		return nullptr;

    if (configExists) {

        // there is only ever 1 config for plugin lights
        for (auto& cfg : *configs) {

            auto backupLightData = light->data;

            LightData::SetTESObjectLightDataFromConfig(light, cfg);

            // TODO:: This is too late to set the emittance it would appear 
            if (cfg.emittanceRegion) {

                    if (auto* form = RE::TESForm::LookupByEditorID(cfg.externalEmittance)) {
                        LightData::SetRefLightEmittanceSource(ref, form);
                    }
                
            }

            // if external emittance is empty, on creation the plugin light never had ext emitt 
            if (cfg.externalEmittance.empty()) {
                LightData::SetRefLightEmittanceSource(ref, nullptr);
            }

            auto* niLight = func(light, ref, node, forceDynamic, useLightRadius, affectRequesterOnly);

            light->data = backupLightData;

            if (!niLight) return niLight;

            //no scale needed to set
            LightData::setNiPointLightDataFromCfg(niLight, cfg, 1.0);

            niLight->name = "OL";

            return niLight;
        }
    }

    auto* niLight = func(light, ref, node, forceDynamic, useLightRadius, affectRequesterOnly);

    if (!niLight) return niLight;

    //mark these lights so i can identify them and create a config for them if user opens the relight menu
    // this allows me to filter placed object ref lights from hazard, magic and actor lights
    niLight->name = "OL"; 

    niLight->fade *= globals::vanillaBrightnessModifier; 

    return niLight; 
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

bool TESObjectLIGH_GenDynamic::shouldDisableLight(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref, std::string& edid, std::string& modName)
{
	if (!ref || !light || ref->IsDynamicForm()) {
		return false;
	}

    if (forms::ContainsEditorID(edid, globals::disableByEditorID)) return true;

    auto formID = ref->GetFormID();

    auto rawIndex = (formID & 0xFF000000) >> 24;
    bool isLight = rawIndex == 0xFE;

    if (!isLight) {
        formID &= 0x00FFFFFF;
    }

    // double use of this vector can also disable vanilla lights its also used to prevent refs from getting relights  
                                                               //remove load order index 
    if (globals::excludedRefFormIDs.contains(formID)) {
        logger::info("excluded ref runtime formID 0x{:08X} relight will not disable this light", static_cast<std::uint32_t>(ref->GetFormID()));
        return false;
    }

    if (!globals::disableGameLights) return false;

	if (forms::ContainsEditorID(edid, globals::enableByEditorID)) return false;

   auto modNameLower = toLowerImmut(modName); 

    for (const auto& whitelistedMod : globals::whitelist) {
        if (modNameLower.find(whitelistedMod) != std::string::npos) {

            logger::info("whitelisted ref 0x{:08X} from mod {} with edid {} relight will not disable this light", static_cast<std::uint32_t>(ref->GetFormID()), whitelistedMod, edid);
            return false;
        }
    }

	return true;
}

//used to disable lights from flora once they are picked.

bool TreeActivateHook::Activate(
    RE::TESObjectTREE* a_this,
    RE::TESObjectREFR* a_targetRef,
    RE::TESObjectREFR* a_activatorRef,
    std::uint8_t a_arg3,
    RE::TESBoundObject* a_object,
    std::int32_t a_targetCount)
{

    if (!a_this) return func(a_this, a_targetRef, a_activatorRef, a_arg3, a_object, a_targetCount);

    logger::debug("tree activate hook fired"); 
    auto player = RE::PlayerCharacter::GetSingleton();

    if (player && a_activatorRef == player && a_targetRef) {
        auto root = a_targetRef->Get3D();

        if (root) {
            if (auto rootNode = root->AsNode()) {
                for (auto& child : rootNode->GetChildren()) {
                    if (!child) {
                        continue;
                    }

                    std::string_view name = child->name.c_str();

                    if (name.size() >= 2 && name[0] == 'R' && name[1] == 'L') {
                        child->SetAppCulled(true);
                        logger::info("culled a plant light");
                    }
                }
            }
        }
    }

    return func(a_this, a_targetRef, a_activatorRef, a_arg3, a_object, a_targetCount);
}

void TreeActivateHook::Install()
{
    REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_TESObjectTREE[0] };

    func = vtbl.write_vfunc(0x37, Activate);

    logger::info("Installed TREE::Activate hook");
}

//used to limit surfaces to 7 closest lights to prevent light flickering
//what I do here is reimplement this base game func, and collect closest lights once 1 second after cell loaded (otherwise no lights yet)
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

    // return here or crash on effect shaders  idk why
    if (p->GetMaterialType() == RE::BSShaderMaterial::Type::kEffect) return true;

    // torches spells we marked in attachlight hooks so they can bypass here
    if (light->unk060 == 4) return true;

    if (!globals::secondAfterCellFullyLoaded.load() || !globals::enableLightFlickerPreventionMeasures || globals::unDesiredMenuOpen.load()) return true;

    auto pass = p->renderPassList.head;
    if (!pass || !pass->geometry) return true;
    
    // dont want to block large tri shapes that might supposed have more then 7 lights (windhelmBridge, candlehearthhall) 
    if (pass->geometry->worldBound.radius > globals::largeSurfaceSize) return true;

    //return on actors and skip invalid references potentially
    auto objectRef = pass->geometry->GetUserData();

    if (!objectRef) {
        return true;
    }

    // dont want to block light on actors
    if (objectRef->IsActor()) return true; 

    std::lock_guard lock(LightData::triLightCacheMutex);
    uint16_t currentGen = LightData::triLightCacheGeneration.load();
    uint16_t storedGen = 0, storedIdx = 0;
    UnpackFD(p->forcedDarkness, storedGen, storedIdx);

    // forced darkness is key to light list, if player switches cells, the pool of cached light lists changes
    if (p->forcedDarkness == 0.0f || storedGen != currentGen) {

        // dont let it go out of bounds (65535)
        if (LightData::triLightCache.size() >= 60000) {
            LightData::triLightCache.clear();
            LightData::triLightCacheGeneration.fetch_add(1);
        }

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

