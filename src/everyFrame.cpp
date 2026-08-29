#include "everyFrame.h"


//this is used for flicker it runs every frame and works with SKSE Menu framework menu opem
void PlayerCharacter_Update::thunk(RE::PlayerCharacter* player, float delta) {

	func(player, delta);

	if (!player) {
		logger::warn("player is null cant check cell");
		return;
	}

	handlePendingMerges();

	// if cell is loaded lets wait 1 second before continuing. 
	if (globals::cellFullyLoaded.load() && !globals::secondAfterCellFullyLoaded.load()) {
		if (OneSecondPassed(globals::cellFullyLoadedTimerStart)) {
			globals::secondAfterCellFullyLoaded.store(true); 
		}
		return; 
	}

	if (globals::secondAfterCellFullyLoaded.load()) {
		// clear so ref can be reprocessed again. for mods like dynamic candles
		std::lock_guard lock(globals::refsWithAttachedLightsMutex);
		globals::refsWithAttachedLights.clear();

		// clear so ref can be reprocessed again. for mods like dynamic candles
		std::lock_guard lock2(globals::mergedRefsMutex);
		globals::mergedRefs.clear();

		// idk if need to make this false again but anyway why not
		//globals::cellFullyLoaded.store(false);
	}

	auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
	if (!ssNode) {
		logger::warn("ShadowSceneNode[0] is null!");
		return;
	}

	auto& ssRt = ssNode->GetRuntimeData();

	const auto playerPos = player->GetPosition(); 

	static float externalEmittanceTimer = 0.0f;
	static bool updateExternalEmittance = false; 
	externalEmittanceTimer += delta;

	if (externalEmittanceTimer >= 1.0f) {
		externalEmittanceTimer = 0.0f;

		updateExternalEmittance = true; 
	}

	updateLights(ssRt.activeLights, delta, false, playerPos, updateExternalEmittance);

	updateLights(ssRt.activeShadowLights, delta, true, playerPos, updateExternalEmittance);

	updateExternalEmittance = false; 

	if (globals::skseMenuOpened) {
		globals::skseMenuOpened = false;
	}
	else if (globals::debugLinesNeedClear) {
		// menu just closed and there are lines to clear
		auto* api = DebugAPI_IMPL::DebugAPI::GetSingleton();
		if (api) {
		
			std::unique_lock lock(api->mutex_);
			for (auto* line : api->LinesToDraw) delete line;
			api->LinesToDraw.clear();
		
		}
		DebugAPI_IMPL::DebugAPI::GetSingleton()->Update();
		globals::debugLinesNeedClear = false; // only clears once
	}
}

void PlayerCharacter_Update::Install()
{
	func = REL::Relocation<std::uintptr_t>(RE::PlayerCharacter::VTABLE[0])
		.write_vfunc(0xAD, thunk);
	logger::info("player chracter update hook installed");
}

//vanillas flicker update func
void NiLightFlickerHook::thunk(RE::TESObjectLIGH* a_lightTemplate, RE::REFR_LIGHT* a_refrLight, RE::TESObjectREFR* a_ref, float a_value) {

	// skip torches i coudlent get it to work (k can carry)
	if (!a_refrLight || !a_lightTemplate)
		return func(a_lightTemplate, a_refrLight, a_ref, a_value);

	RE::NiLight* light = a_refrLight->light.get();

	if (!light) return func(a_lightTemplate, a_refrLight, a_ref, a_value);

	auto& rt = light->GetLightRuntimeData();

	auto it = LightData::configIDToJsonCfg.find(rt.unk138);
	if (it == LightData::configIDToJsonCfg.end())
		return func(a_lightTemplate, a_refrLight, a_ref, a_value);

	const auto& config = it->second;

	float backupFade = a_lightTemplate->fade;
	float backupMovement = a_lightTemplate->data.flickerMovementAmplitude;
	float backupFlickerRate = a_lightTemplate->data.flickerPeriodRecip;
	float backupIntensity = a_lightTemplate->data.flickerIntensityAmplitude;

	a_lightTemplate->fade = config.startingFade * globals::vanillaBrightnessModifier;
	a_lightTemplate->data.flickerMovementAmplitude = config.flickerAmplitude;
	a_lightTemplate->data.flickerPeriodRecip = config.flickersPerSecond;
	a_lightTemplate->data.flickerIntensityAmplitude = config.flickerIntensity;

	func(a_lightTemplate, a_refrLight, a_ref, a_value);

	RE::NiPoint3 flickerOffset = light->local.translate;
	light->local.translate.x = config.position[0] + flickerOffset.x;
	light->local.translate.y = config.position[1] + flickerOffset.y;
	light->local.translate.z = config.position[2] + flickerOffset.z;

	a_lightTemplate->fade = backupFade;
	a_lightTemplate->data.flickerMovementAmplitude = backupMovement;
	a_lightTemplate->data.flickerPeriodRecip = backupFlickerRate;
	a_lightTemplate->data.flickerIntensityAmplitude = backupIntensity;

	//Force a fresh transform/bounds update now so world-space position actually reflects the fix.
	RE::NiUpdateData updateData{};
	updateData.time = 0.0f;
	updateData.flags = RE::NiUpdateData::Flag::kDirty;

	if (auto* parent = light->parent) {
		parent->UpdateTransformAndBounds(updateData);
	}
}

// original func SE 14021f660 whos id is 17212

void NiLightFlickerHook::Install()
{
	// JMP to original AE FUN_14026f480 - ID: 17613
	// JMP to original SE FUN_14021f640 - ID: 17211
	// The JMP to TESObjectLIGH::UpdateFlicker is at offset 0xA

	REL::Relocation<std::uintptr_t> target(
		RELOCATION_ID(17211, 17613),
		REL::VariantOffset{ 0xA, 0xA, 0xA });

	func = target.write_branch<5>(thunk);

	logger::info("NiLightFlickerHook installed at 0x{:X}", target.address());

	// PlayerCharacter::UpdateRefLight
	
	// AE ID: 40570, Address: 140742180
	//SE ID: 39491 Address: 1406ad800

	REL::Relocation<std::uintptr_t> playerRefLight(
		RELOCATION_ID(39491, 40570),
		REL::VariantOffset{ 0x1C7, 0x1C7, 0x1C7 });

	playerRefLight.write_call<5>(thunk);

	logger::info("NiLightFlickerHook PlayerCharacter call installed at 0x{:X}",
		playerRefLight.address());
}