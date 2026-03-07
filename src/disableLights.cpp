
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

    if (!light->affectLand)
    {
        const auto& flags = p->flags;
        if (flags.any(RE::BSShaderProperty::EShaderPropertyFlag::kMultiTextureLandscape) ||
            flags.any(RE::BSShaderProperty::EShaderPropertyFlag::kNoLODLandBlend))
        {
            return false;
        }
    }

    //exit in exteriors
    if (!globals::currentCellIsInterior || !globals::enableLightFlickerPreventionMeasures) return true;

    auto pass = p->renderPassList.head;
    if (!pass || !pass->geometry) return false;


    auto geometry = pass->geometry;
    const auto& triCenter = geometry->worldBound.center;
    const float triRadius = geometry->worldBound.radius;


    const auto& lightPos = light->light->world.translate;


    const float dist = triCenter.GetDistance(lightPos);

    if (light->unk060 == 1) {

        const float dx = std::abs(lightPos.x - triCenter.x);
        const float dy = std::abs(lightPos.y - triCenter.y);

        // Ignore Z almost entirely for candles
        const float distXY = std::sqrt(dx * dx + dy * dy);


        if (distXY > globals::gMinChandelierCoverage)
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

            if (zDiff > globals::g_maxShadowCompeteDistance)
                continue;

            float otherDistance = triCenter.GetDistance(otherLight->light->world.translate);
            float otherCoverage = otherLight->light->radius.Length() - (otherDistance - triRadius);

            /*logger::debug(
                 "light {:p} dist {:.2f} vs other {:p} dist {:.2f}",
                 static_cast<void*>(light),
                 thisDistance,
                 static_cast<void*>(otherLight.get()),
                 otherDistance
             );
             */
            if (otherCoverage > thisCoverage) {
                return false;
            }

        }

    }

    //candles
    else if (light->unk060 == 2)
    {
        auto isWallMesh = false;

        RE::TESBoundObject* baseRef = nullptr;

        if (p->fadeNode && p->fadeNode->userData)
            baseRef = p->fadeNode->userData->data.objectReference;

        if (baseRef) {
            auto curBaseFormID = baseRef->GetFormID();
            isWallMesh = globals::wallMeshes.contains(curBaseFormID);
        }

        const float dx = std::abs(lightPos.x - triCenter.x);
        const float dy = std::abs(lightPos.y - triCenter.y);

        const float distXY = std::sqrt(dx * dx + dy * dy);

        float coverageThreshold = globals::gMinCandleCoverage;

        if (isWallMesh && triRadius < globals::maxWallSizeForStrictLightBounds) {

            coverageThreshold = globals::minCandleCoverageWall;
        }


        if (distXY > coverageThreshold)
            return false;
    }


    //fires
    else if (light->unk060 == 3) {

        auto isWallMesh = false;

        RE::TESBoundObject* baseRef = nullptr;

        if (p->fadeNode && p->fadeNode->userData)
            baseRef = p->fadeNode->userData->data.objectReference;

        if (baseRef) {
            auto curBaseFormID = baseRef->GetFormID();
            isWallMesh = globals::wallMeshes.contains(curBaseFormID);
        }

        const float dx = std::abs(lightPos.x - triCenter.x);
        const float dy = std::abs(lightPos.y - triCenter.y);

        const float distXY = std::sqrt(dx * dx + dy * dy);

        float coverageThreshold = globals::gMinFireCoverage;

        if (isWallMesh && triRadius < globals::maxWallSizeForStrictLightBounds) {

            coverageThreshold = globals::gMinFireCoverageWall;
        }

        if (distXY > coverageThreshold)
            return false;
    }

    else {
        if (dist > globals::globalCoverage)
            return false;

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

