#include "LightAttachmentHooks.h"
#include "forms.h"
#include "disableLights.h"

//attach light to spells explosions effects and the likes, no light merging and exact mesh paths required
namespace ObjectReference
{
    template <class T>
    RE::NiAVObject* Load3D<T>::thunk(T* a_this, bool a_backgroundLoading)
    {
        auto niAVObject = func(a_this, a_backgroundLoading);
        if (!niAVObject || !a_this) {
            return niAVObject;
        }

        RE::FormID refFormID = a_this->GetFormID();

        bool dontAttachedDebugMarker = true;

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

        const auto baseObject = a_this->GetBaseObject();
        if (!baseObject) return niAVObject;

        const auto baseFormID = baseObject->GetFormID();

        if (auto* baseCfgs = LightData::findConfigsByFormID(baseFormID, isInterior, true)) {
            if (baseCfgs->empty() || forms::isExcludedRef(a_this)) return niAVObject;

            for (const auto& cfg : *baseCfgs) {
                auto* light = LightManager::AttachLight(
                    cfg, a_root, a_this, cfg.menuName, refFormID, dontAttachedDebugMarker);

                if (!light) {
                    logger::warn("AttachLight failed for ref {:08X} with light '{}'", refFormID, cfg.menuName);
                }
            }

            return niAVObject;
        }

        const auto bm = baseObject->As<RE::TESModel>();
        if (!bm) return niAVObject;

        auto currentModel = std::string(bm->GetModel());
        auto meshName = extractMeshName(currentModel);
        toLower(meshName);

        logger::debug(" misc load 3d meshName loaded {}", meshName);

      const auto& cfgs = findConfigsForMeshPath(meshName, isInterior);
   
       if (cfgs.empty()) return niAVObject;

       else {

           for (auto& cfg : cfgs) {
               auto* light = LightManager::AttachLight(
                   cfg,
                   a_root,
                   a_this,
                   meshName,
                   refFormID,
                   dontAttachedDebugMarker);

               if (!light) {
                   logger::warn("AttachLight failed for ref {:08X} with mesh '{}'", refFormID, meshName);
                   continue;
               }
           }

       }

        return niAVObject;
    }

    template <class T>
    void ObjectReference::Load3D<T>::Install()
    {
        func = REL::Relocation<std::uintptr_t>(T::VTABLE[0])
            .write_vfunc(idx, thunk);

        logger::info("Hooked {}::Load3D", typeid(T).name());
    }

    template struct Load3D<RE::BarrierProjectile>;
    template struct Load3D<RE::BeamProjectile>;
    template struct Load3D<RE::ConeProjectile>;
    template struct Load3D<RE::MissileProjectile>;
    template struct Load3D<RE::Hazard>;
    template struct Load3D<RE::Explosion>;
    template struct Load3D<RE::ArrowProjectile>;
    template struct Load3D<RE::GrenadeProjectile>;

    void InstallLoad3DHooks()
    {
        Load3D<RE::BarrierProjectile>::Install();
        Load3D<RE::BeamProjectile>::Install();
        Load3D<RE::ConeProjectile>::Install();
        Load3D<RE::MissileProjectile>::Install();
        Load3D<RE::Hazard>::Install();
        Load3D<RE::Explosion>::Install();
        Load3D<RE::ArrowProjectile>::Install();
        Load3D<RE::GrenadeProjectile>::Install();
    }
}

//attach light to static objects, allows light merging and partial mesh path search
RE::NiAVObject* TESObjectREFRLoad3D::thunk(RE::TESObjectREFR* a_this, bool a_backgroundLoading)
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

    // ore veins ect 
    if (a_this->IsDestroyed()) {
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

void TESObjectREFRLoad3D::Install()
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

    if (!configExists && shouldDisableLight(light, ref, edid, modName, false))
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
        std::make_pair(RELOCATION_ID(15527, 15704), 0xAC),   // AE FUN_140217160 / SE FUN_1401ca8d0 // add on nodes (torches ect)
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

    if (!configExists && shouldDisableLight(light, ref, edid, modName, false))
        return nullptr;

    if (configExists) {

        // there will never be more then 1 for this. 
        for (auto& cfg : *configs) {
            auto backupLightData = light->data;
            LightData::SetTESObjectLightDataFromConfig(light, cfg);

            auto* niLight = magicLightFunc(light, ref, node, forceDynamic, useLightRadius, affectRequesterOnly);
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
    auto* niLight = magicLightFunc(light, ref, node, forceDynamic, useLightRadius, affectRequesterOnly);
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

    if (!a_targetRef || !a_this) {
        return result;
    }

    logger::debug("activate called");

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

// attach lights to ShaderReferenceEffect on Init
// attach lights to ShaderReferenceEffect on Init
namespace ReferenceEffect
{
    bool Init::thunk(RE::ShaderReferenceEffect* a_this)
    {
        auto result = func(a_this);

        if (!result || !a_this || !a_this->effectData) {
            return result;
        }

        const auto ref = a_this->target.get();
        if (!ref) {
            return result;
        }

        auto rootNiAV = GetReferenceAttachRoot(a_this);
        if (!rootNiAV) {
            return result;
        }

        // Match the first-person attach root to the third-person weapon root
        // when available.
        if (const auto thirdPerson3D = ref->Get3D(false)) {
            if (const auto thirdPersonRoot =
                thirdPerson3D->GetObjectByName(rootNiAV->name)) {
                rootNiAV = thirdPersonRoot;
            }
        }

        auto root = netimmerse_cast<RE::NiNode*>(rootNiAV);
        if (!root) {
            return result;
        }

        // Ignore temporary inventory references.
        if (auto invMgr = RE::Inventory3DManager::GetSingleton();
            invMgr && invMgr->tempRef == ref.get()) {
            return result;
        }

        bool dontAttachDebugMarker = true;

        // Look for an existing enchantment light.
        auto findExistingLight = [](RE::NiAVObject* root3D) -> RE::NiPointLight* {
            if (!root3D) {
                return nullptr;
            }

            // see if light exists already we dont wanna attach like 30 lights after player has seathed/ equipped alot of times
            std::function<RE::NiPointLight* (RE::NiAVObject*)> findLight =
                [&](RE::NiAVObject* node) -> RE::NiPointLight* {
                if (!node) {
                    return nullptr;
                }

                if (auto* light = netimmerse_cast<RE::NiPointLight*>(node)) {
                    const char* name = light->name.c_str();

                    if (name &&
                        name[0] == 'R' &&
                        name[1] == 'L' &&
                        light->fadeAmount == 5.0f) {
                        return light;
                    }
                }

                if (auto* niNode = node->AsNode()) {
                    for (auto& child : niNode->children) {
                        if (auto* light = findLight(child.get())) {
                            return light;
                        }
                    }
                }

                return nullptr;
                };

            return findLight(root3D);
            };

        const RE::FormID formID = a_this->effectData->GetFormID();

        auto effectEditorID =
            clib_util::editorID::get_editorID(a_this->effectData);

        logger::debug(
            "ShaderReferenceEffect: shader FormID={:08X}, EditorID='{}'",
            formID,
            effectEditorID);

        if (auto configs = LightData::findConfigsByFormID(
            formID,
            true,
            true);
            configs && !configs->empty()) {

            logger::info(
                "Found {} config(s) for effect shader FormID {:08X}",
                configs->size(),
                formID);

            for (auto& cfg : *configs) {

                if (auto* existingLight = findExistingLight(root)) {
                    logger::debug(
                        "Reusing existing enchantment light '{}' on ref {:08X}",
                        existingLight->name.c_str(),
                        ref->GetFormID());

                    existingLight->SetAppCulled(false);
                    continue;
                }

                auto* light = LightManager::AttachLight(
                    cfg,
                    root,
                    ref.get(),
                    cfg.menuName,
                    formID,
                    dontAttachDebugMarker);

                if (light) {
                    light->fadeAmount = 5;
                }

                if (!light) {
                    logger::warn(
                        "AttachLight failed for ref {:08X} with light '{}'",
                        ref->GetFormID(),
                        cfg.menuName);
                }
            }

            return result;
        }

        if (effectEditorID.empty()) {
            logger::warn(
                "Effect shader {:08X} has no EditorID",
                formID);

            return result;
        }

        toLower(effectEditorID);

        logger::debug(
            "No FormID config found. Trying shader EditorID '{}'",
            effectEditorID);

        auto cell = ref->GetParentCell();

        if (!cell) {
            logger::warn(
                "ShaderReferenceEffect {:08X}: target has no parent cell",
                formID);

            return result;
        }

        const bool isInterior = cell->IsInteriorCell();

        //lazy implememntation we use editor id as mesh path as i never implemented a editor ID json entry for relight
        const auto& configs =
            findConfigsForMeshPath(effectEditorID, isInterior);

        if (configs.empty()) {
            logger::warn(
                "No configs found for shader EditorID '{}'",
                effectEditorID);

            return result;
        }

        logger::info(
            "Found {} config(s) for shader EditorID '{}'",
            configs.size(),
            effectEditorID);

        for (auto& cfg : configs) {

            if (auto* existingLight = findExistingLight(root)) {
                logger::debug(
                    "Reusing existing enchantment light '{}' on ref {:08X}",
                    existingLight->name.c_str(),
                    ref->GetFormID());

                existingLight->SetAppCulled(false);
                continue;
            }

            auto* light = LightManager::AttachLight(
                cfg,
                root,
                ref.get(),
                effectEditorID,
                formID,
                dontAttachDebugMarker);

            if (light) {
                light->fadeAmount = 5;
            }

            if (!light) {
                logger::warn(
                    "AttachLight failed for ref {:08X} with shader '{}'",
                    ref->GetFormID(),
                    effectEditorID);
            }
        }

        return result;
    }

    void Init::Install()
    {
        func = REL::Relocation<std::uintptr_t>(
            RE::ShaderReferenceEffect::VTABLE[0])
            .write_vfunc(idx, thunk);

        logger::info("Hooked RE::ShaderReferenceEffect::Init");
    }

    void Install()
    {
        Init::Install();
    }
}



 