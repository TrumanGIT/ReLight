
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

    //light list is 0 on first calls. must ddelay
    //lightevalstart timer is started in light disable lights.h cell fully loaded event 
    
        if (!globals::cellFullyLoaded) return true; 


        /*    if (p->forcedDarkness == 0.0f) {
     p->forcedDarkness = -1.0f;

            std::lock_guard lock(PendingProperty::mutex);
            PendingProperty::list.push_back({ RE::NiPointer<RE::BSLightingShaderProperty>(p), std::chrono::steady_clock::now()});
          
          //  logger::debug("[LightHook] Initial forcedDarkness set: {}", static_cast<int>(p->forcedDarkness) - 1);
            return true; 
        }*/ 


    // first frame only: initialize forcedDarkness
    //exit in exteriors and while the 
    if (  /*p->forcedDarkness < globals::flickerPreventionLightLimit ||*/  !globals::currentCellIsInterior || !globals::enableLightFlickerPreventionMeasures) return true;

    auto pass = p->renderPassList.head;
    if (!pass || !pass->geometry) return false;

    auto geometry = pass->geometry;
    const auto& triCenter = geometry->worldBound.center;
    const float triRadius = geometry->worldBound.radius;

    const auto& lightPos = light->light->world.translate;

    switch (light->unk060)
    {

        //TODO:: capture its closest light once not every call
    case 1: // candles
    {
        bool isWallMesh = false;

        RE::TESBoundObject* baseRef = nullptr;
        if (p->fadeNode && p->fadeNode->userData)
            baseRef = p->fadeNode->userData->data.objectReference;

        if (baseRef)
            isWallMesh = globals::wallMeshes.contains(baseRef->GetFormID());

        const float dx = std::abs(lightPos.x - triCenter.x);
        const float dy = std::abs(lightPos.y - triCenter.y);
        const float distXY = std::sqrt(dx * dx + dy * dy);

        float coverageThreshold = globals::minCandleCoverage;

        if (triRadius < globals::maxWallSizeForStrictLightBounds)
            coverageThreshold = globals::minCandleCoverageSM;

        if (isWallMesh && triRadius < globals::maxWallSizeForStrictLightBounds)
            coverageThreshold = globals::minCandleCoverageWall;

        if (distXY > coverageThreshold)
            return false;
        break;
    }

    case 2: // chandeliers
    {
        const float dx = std::abs(lightPos.x - triCenter.x);
        const float dy = std::abs(lightPos.y - triCenter.y);
        const float distXY = std::sqrt(dx * dx + dy * dy);

        if (distXY > globals::minChandelierCoverage)
            return false;

        const auto& thisLightPos = light->light->world.translate;
        float thisDistance = triCenter.GetDistance(thisLightPos);
        float thisRadius = light->light->radius.Length();
        float thisCoverage = thisRadius - (thisDistance - triRadius);

        auto thisConfigID = light->light->unk138;
        auto* ss = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];

        for (auto& otherLight : ss->GetRuntimeData().activeShadowLights) {
            if (!otherLight || !otherLight->light)
                continue;
            if (otherLight->light == light->light)
                continue;
            if (otherLight->light->unk138 != thisConfigID)
                continue;

            float zDiff = std::abs(thisLightPos.z - otherLight->light->world.translate.z);

            if (zDiff > globals::maxShadowCompeteDistance)
                continue;

            float otherDistance = triCenter.GetDistance(otherLight->light->world.translate);
            float otherCoverage = otherLight->light->radius.Length() - (otherDistance - triRadius);

            if (otherCoverage > thisCoverage)
                return false;
        }

        break;
    }

    case 3: // fires
    {
        bool isWallMesh = false;

        RE::TESBoundObject* baseRef = nullptr;
        if (p->fadeNode && p->fadeNode->userData)
            baseRef = p->fadeNode->userData->data.objectReference;

        if (baseRef)
            isWallMesh = globals::wallMeshes.contains(baseRef->GetFormID());

        const float dx = std::abs(lightPos.x - triCenter.x);
        const float dy = std::abs(lightPos.y - triCenter.y);
        const float distXY = std::sqrt(dx * dx + dy * dy);

        float coverageThreshold = globals::minFireCoverage;

        if (isWallMesh && triRadius < globals::maxWallSizeForStrictLightBounds)
            coverageThreshold = globals::minFireCoverageWall;

        if (distXY > coverageThreshold)
            return false;

        break;
    }

    default: // anything besides candles fires or chandelliers
    { 
        // exclude large lights like bleak falls barrow01 sunlight
        if (light->light->radius.x < 1000) {
            const float dist = triCenter.GetDistance(lightPos);

            if (dist > globals::globalCoverage)
                return false;
        }

        break;
    }
    }


    return true;
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

