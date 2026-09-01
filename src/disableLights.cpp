
#include "disableLights.h"
#include "Utility.h"
#include "LightManager.h"
#include "forms.hpp"
#include "config.hpp"


//Po3's hook THIS DISABLES ALL LIGHTS TO START WITH A CLEAN BASE TO WORK FROM

//EDIT i now use this hook also to edit lights from  a esp, esm, esl 
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

    // For torches/lanterns (CanBeCarried), use the light's own FormID and base ID lookup
    RE::FormID searchFormID = ref->GetFormID();
    bool isBaseID = false;
    bool canBeCarried = light->CanBeCarried();
    std::string modName = "";
    
    // equippable lights user data is the player itself so we must use the base light object owner file
    if (canBeCarried) {
        // Torch/lantern: use the light template's data
        const RE::TESFile* baseOriginFile = light->GetDescriptionOwnerFile();
        modName = baseOriginFile ? baseOriginFile->fileName : "";
        searchFormID = light->GetFormID();
        isBaseID = true; 

        if (modName.empty()) {
            logger::warn("Torch: baseOriginFile is null, using 'Skyrim.esm' as fallback");
            modName = "Skyrim.esm";
        }
    }
    // use the ref as the base origin file if its not a equipable ligh
    else {
        const RE::TESFile* refOriginFile = ref->GetDescriptionOwnerFile();
        modName = refOriginFile ? refOriginFile->fileName : "";
    }

    // Find config using the appropriate FormID and isBaseID flag
    auto configs = LightData::findConfigsByFormID(searchFormID, true, isBaseID);
    bool configExists = configs != nullptr && !configs->empty();

    if (!configExists && shouldDisableLight(light, ref, edid, modName))
        return nullptr;

    if (configExists) {

        for (auto& cfg : *configs) {
            auto backupLightData = light->data;
            LightData::SetTESObjectLightDataFromConfig(light, cfg);

            if (cfg.emittanceRegion) {
                if (auto* form = RE::TESForm::LookupByEditorID(cfg.externalEmittance)) {
                    LightData::SetPluginLightEmittanceSource(ref, form);
                }
            }

            if (cfg.externalEmittance.empty()) {
                LightData::SetPluginLightEmittanceSource(ref, nullptr);
            }

            auto* niLight = func(light, ref, node, forceDynamic, useLightRadius, affectRequesterOnly);
            light->data = backupLightData;

            if (!niLight) return niLight;

            LightData::setNiPointLightDataFromCfg(niLight, cfg, 1.0);
            niLight->name = "ol";

            // mark 4 so can be excluded in light flicker prevention (IsLightAffectingSurface Hook)
            if (canBeCarried) niLight->fadeAmount = 4; 

            return niLight;
        }
        return func(light, ref, node, forceDynamic, useLightRadius, affectRequesterOnly);
    }

    // No config exists - create the light normally
    auto* niLight = func(light, ref, node, forceDynamic, useLightRadius, affectRequesterOnly);
    if (!niLight) return niLight;

    // For torches/lanterns, create a config in memory so can be edited in the light editor
    if (canBeCarried) {

        LightConfig cfg;
        CreateConfigFromPluginLight(cfg, niLight, light, ref, edid, modName, true);

        niLight->unk138 = cfg.configID;
        niLight->fadeAmount = 4;

    }

    niLight->name = "ol";
    niLight->fade *= globals::vanillaBrightnessModifier;

    return niLight;
}

RE::NiPointLight* TESObjectLIGH_GenDynamic::magicLightThunk(
    RE::TESObjectLIGH* light,
    RE::TESObjectREFR* ref,
    RE::NiNode* node,
    bool forceDynamic,
    bool useLightRadius,
    bool affectRequesterOnly)
{
    if (!ref || !light)
        return magicLightFunc(light, ref, node, forceDynamic, useLightRadius, affectRequesterOnly);

    std::string edid = clib_util::editorID::get_editorID(light);

   // For torches/lanterns (CanBeCarried), use the light's own FormID and base ID lookup
    RE::FormID formID = light->GetFormID();

    // equippable lights user data is the player itself so we must use the base light object owner file
        // Torch/lantern: use the light template's data
       const RE::TESFile* baseOriginFile = light->GetDescriptionOwnerFile();
        std::string  modName = baseOriginFile ? baseOriginFile->fileName : "";

        if (modName.empty()) {
            logger::warn("baseOriginFile is null");
            modName = "Skyrim.esm";
        }

    // Find config using the appropriate FormID and isBaseID flag
    auto configs = LightData::findConfigsByFormID(formID, true, true);
    bool configExists = configs != nullptr && !configs->empty();

    if (!configExists && shouldDisableLight(light, ref, edid, modName))
        return nullptr;

    if (configExists) {

        // there will never be more then 1 for this. 
        for (auto& cfg : *configs) {
            auto backupLightData = light->data;
            LightData::SetTESObjectLightDataFromConfig(light, cfg);

            auto* niLight = func(light, ref, node, forceDynamic, useLightRadius, false);
            light->data = backupLightData;

            if (!niLight) return niLight;

            LightData::setNiPointLightDataFromCfg(niLight, cfg, 1.0);
            niLight->name = "ol";

            // mark 4 so can be excluded in light flicker prevention (IsLightAffectingSurface Hook)
           niLight->fadeAmount = 4;

            return niLight;
        }
       return magicLightFunc(light, ref, node, forceDynamic, useLightRadius, affectRequesterOnly);
    }

    // No config exists - create the light normally
    auto* niLight = magicLightFunc(light, ref, node, forceDynamic, useLightRadius, false);
    if (!niLight) return niLight;

    LightConfig cfg;
    CreateConfigFromPluginLight(cfg, niLight, light, ref, edid, modName, true);

     niLight->unk138 = cfg.configID;

    niLight->name = "ol";
    niLight->fade *= globals::vanillaBrightnessModifier;

    niLight->fadeAmount = 4;

    return niLight;
}


void TESObjectLIGH_GenDynamic::Install()
{
    auto& trampoline = SKSE::GetTrampoline();

    std::array targets{
        std::make_pair(RELOCATION_ID(17206, 17603), 0x1D3),  // TESObjectLIGH::Clone3D 14026E950
        std::make_pair(RELOCATION_ID(19252, 19678), 0xB8),   // TESObjectREFR::AddLight  1402E12F0
        std::make_pair(RELOCATION_ID(15527, 15704), 0xAC),   // AE FUN_140217160 / SE FUN_1401ca8d0
    };

    for (const auto& [address, offset] : targets) {
        REL::Relocation<std::uintptr_t> target{ address, offset };

        TESObjectLIGH_GenDynamic::func =
            trampoline.write_call<5>(
                target.address(),
                TESObjectLIGH_GenDynamic::thunk);
    }

    logger::info("Installed TESObjectLIGH::GenDynamic patches");
}

void TESObjectLIGH_GenDynamic::MagicLightThunkInstall()
{
    auto& trampoline = SKSE::GetTrampoline();

    if (REL::Module::IsAE()) {
        logger::info("IsAE(): {}", REL::Module::IsAE());
        REL::Relocation<std::uintptr_t> target{
            RELOCATION_ID(33403, 34185),
            REL::VariantOffset{ 0x407, 0x407, 0x407 }
        };

        TESObjectLIGH_GenDynamic::magicLightFunc =
            trampoline.write_call<5>(
                target.address(),
                TESObjectLIGH_GenDynamic::magicLightThunk);
    }

    std::array magicTargets{
     std::make_pair(
         RELOCATION_ID(33603, 34381),
         REL::VariantOffset{ 0xAC, 0xE2, 0xE2 }),

     std::make_pair(
         RELOCATION_ID(33391, 34151),
         REL::VariantOffset{ 0x86, 0xCD, 0x86 }),
    };

    for (const auto& [address, offset] : magicTargets) {
        REL::Relocation<std::uintptr_t> target{ address, offset };

        TESObjectLIGH_GenDynamic::magicLightFunc =
            trampoline.write_call<5>(
                target.address(),
                TESObjectLIGH_GenDynamic::magicLightThunk);
    }

    // std::make_pair(RELOCATION_ID(33603, 34379), 0xAC), //1405BAB10
//  std::make_pair(RELOCATION_ID(0, 34381), 0xE2),// 1405BACD0
    //   std::make_pair(RELOCATION_ID(0, 34172), 0x86),
    //std::make_pair(RELOCATION_ID(0, 44222), 0x36D), // last caller called when casting flame spell 
    // std::make_pair(RELOCATION_ID(0, 44146), 0x5C), // FUN_1407e7500
    //1407d3920 hit effect related 
    logger::info("Installed TESObjectLIGH::GenDynamic patches");
}


bool TESObjectLIGH_GenDynamic::shouldDisableLight(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref, std::string& edid, std::string& modName)
{
	if (!ref || !light || ref->IsDynamicForm() || light->CanBeCarried()) {
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

    // return here or crash on effect shaders idk why
    if (p->GetMaterialType() == RE::BSShaderMaterial::Type::kEffect) return true;

    // torches spells we marked in attachlight hooks so they can bypass here
    if (light->light->fadeAmount == 4) return true;

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

