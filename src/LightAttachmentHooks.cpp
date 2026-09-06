#include "LightAttachmentHooks.h"
#include "forms.hpp"
#include "disableLights.h"

// ATTACH LIGHTS TO MESHES DURING LOAD3D() HOOK
RE::NiAVObject* Load3D::thunk(RE::TESObjectREFR* a_this, bool a_backgroundLoading)
{

	//logger::info("load3D called");
	auto niAVObject = func(a_this, a_backgroundLoading);
	if (!niAVObject || !a_this) {
		//logger::warn("no ni node casted from niav object from load3d hook");
		return niAVObject;
	}

	RE::FormID refFormID = a_this->GetFormID();

	// ref already has a light placed, introduced to skip over refs that got a merged light
	{
		std::lock_guard lock(globals::refsWithAttachedLightsMutex);
		if (globals::refsWithAttachedLights.count(refFormID) > 0)
			return niAVObject;
	}
	{
		std::lock_guard lock(globals::mergedRefsMutex);
		if (globals::mergedRefs.count(refFormID) > 0)
			return niAVObject;
	}

	// calling asNode crashed on some dyndolod references for a user so netimmersive cast instead
	auto a_root = netimmerse_cast<RE::NiNode*>(niAVObject);
	if (!a_root) {
		return niAVObject;
	}

	auto cell = a_this->GetParentCell();

	if (!cell) {
		logger::warn("no cell cant determine if should use exterior or interior configs");
		return niAVObject;
	}

	bool isInterior = cell->IsInteriorCell();

	// skip harvested plants
	if (a_this->formFlags & (1 << 13)) {
		logger::debug("skip attaching light to harvested plant");
		return niAVObject;
	}

	const auto baseObject = a_this->GetBaseObject();
	if (!baseObject) return niAVObject;

	const auto baseFormID = baseObject->GetFormID();

	//skips fires with base ids below with animations off 
	// 1. Sky Haven chain activated fires
	// 2. Castle Volkihar fires that turn on
	if (!LightManager::IsAnimationsOn(a_this, baseFormID)) {
		return niAVObject;
	}

	// this looks for refs
	if (auto* refCfgs = LightData::findConfigsByFormID(refFormID, isInterior, false)) {

		if (!refCfgs) return niAVObject;

		bool alreadyAttachedDebugMarker = false;

		for (const auto& cfg : *refCfgs) {

			if (cfg.isPluginLight) return niAVObject;

			auto* light = LightManager::AttachLight(
				cfg,
				a_root,
				a_this,
				cfg.menuName,
				refFormID,
				alreadyAttachedDebugMarker);

			if (!light) {
				logger::warn("AttachLight failed for ref {:08X} with light '{}'", refFormID, cfg.menuName);
			}

			globals::baseFormsWithAttachedLights.emplace(baseFormID);
		}

		if (globals::removeFakeGlowOrbs) {
				glowOrbRemover(a_root);
		}
		
		return niAVObject;
	}

	// this looks for base
	if (auto* baseCfgs = LightData::findConfigsByFormID(baseFormID, isInterior, true)) {

		if (!baseCfgs || baseCfgs->empty() || forms::isExcludedRef(a_this)) return niAVObject;

		if (globals::removeFakeGlowOrbs) {
			glowOrbRemover(a_root);
		}

		globals::baseFormsWithAttachedLights.emplace(baseFormID);

		bool alreadyAttachedDebugMarker = false;

		const auto allowLightMerge = baseCfgs->front().shadowLight ? globals::enableShadowLightMerging : globals::enableLightMerging;

		const auto isMultiLight = baseCfgs->size() > 1;

		logger::info(
			"BASE MERGE CHECK {:08X}: configs={}, shadow={}, allowMerge={}, noMerging={}",
			baseFormID,
			baseCfgs->size(),
			baseCfgs->front().shadowLight,
			allowLightMerge,
			LightData::HasRelightFlag(
				baseCfgs->front().flags,
				RELIGHT_FLAGS::kNoMerging));

		//configs with more then 1 light in the json object should not merge
		if (!isMultiLight && allowLightMerge && !LightData::HasRelightFlag(baseCfgs->front().flags, RELIGHT_FLAGS::kNoMerging)) {
			auto cloneLight = LightManager::cloneNiPointLight(LightData::masterNiPointLight.light.get());
			if (!cloneLight) {
				logger::warn("Failed to clone NiPointLight for base object {:08X} )", baseFormID);
				return niAVObject;
			}

			LightManager::fillPendingMerges(a_this, cloneLight, baseCfgs->front(), a_root, false);
			return niAVObject;
		}

		for (const auto& cfg : *baseCfgs) {

			auto* light = LightManager::AttachLight(
				cfg,
				a_root,
				a_this,
				cfg.menuName,
				refFormID,
				alreadyAttachedDebugMarker);

			if (!light) {
				logger::warn("AttachLight failed for ref {:08X} with light '{}'", refFormID, cfg.menuName);
			}

	

		}

		return niAVObject;
	}
	
	const auto bm = baseObject->As<RE::TESModel>();
	if (!bm) return niAVObject;

	auto currentModel = std::string(bm->GetModel());

	//turn mesh name from //statics//whiterun//objects//fires.nif -> fires
	auto meshName = extractMeshName(currentModel);

	//mutable
	toLower(meshName);

	if (LightManager::processByFilePath(a_this, meshName, a_root, isInterior)) {
		globals::baseFormsWithAttachedLights.emplace(baseFormID);

			if (globals::removeFakeGlowOrbs) {
				glowOrbRemover(a_root);
			}
		
		return niAVObject;
	}

	return niAVObject;
}

void Load3D::Install()
{
	func = REL::Relocation<std::uintptr_t>(RE::TESObjectREFR::VTABLE[0])
		.write_vfunc(idx, thunk);
	logger::info("Hooked TESObjectREFR::Load3D");
}

//PO3's hook used to disable and or edit vanilla / modded esp,esm,esl plugin lights
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

         //14074ddc0 called for flame spell 
       std::make_pair(RELOCATION_ID(42965, 44222), REL::VariantOffset{ 0x58, 0x36D, 0x58 }),

           //  std::make_pair(RELOCATION_ID(33603, 34379), 0xAC), //1405BAB10
  //std::make_pair(RELOCATION_ID(0, 34381), 0xE2),// 1405BACD0
    //   std::make_pair(RELOCATION_ID(0, 34172), 0x86),
    // std::make_pair(RELOCATION_ID(0, 44146), 0x5C), // FUN_1407e7500
    };

    for (const auto& [address, offset] : magicTargets) {
        REL::Relocation<std::uintptr_t> target{ address, offset };

        TESObjectLIGH_GenDynamic::magicLightFunc =
            trampoline.write_call<5>(
                target.address(),
                TESObjectLIGH_GenDynamic::magicLightThunk);
    }

    logger::info("Installed TESObjectLIGH::GenDynamic patches");
}


bool Activate::thunk(
	RE::TESObjectACTI* a_this,
	RE::TESObjectREFR* a_targetRef,
	RE::TESObjectREFR* a_activatorRef,
	std::uint8_t a_arg3,
	RE::TESBoundObject* a_object,
	std::int32_t a_targetCount)
{

	bool result = func(a_this, a_targetRef, a_activatorRef, a_arg3, a_object, a_targetCount);

	if (!a_targetRef ||! a_this) {
	//	logger::info("a_targetRef is nullptr");
		return result;
	}

	LightManager::HandleSkyHavenTempleScriptedFires(a_targetRef); 

	LightManager::HandleDLC1VCDungeonScriptedFires(a_targetRef); 

	return result;
}
void Activate::Install()
{
	func = REL::Relocation<std::uintptr_t>(RE::TESObjectACTI::VTABLE[0])
		.write_vfunc(idx, thunk);
	logger::info("Hooked TESObjectACTI::Activate");
}