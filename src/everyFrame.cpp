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

	ApplyLightFlicker(ssRt.activeLights, delta, false, playerPos);

	ApplyLightFlicker(ssRt.activeShadowLights, delta, true, playerPos);

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