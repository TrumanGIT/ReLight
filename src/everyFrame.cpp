#include "everyFrame.h"
#include "LightManager.h"
#include "disableLights.h"

//this is used for flicker it runs every frame and works with SKSE Menu framework menu opem
void PlayerCharacter_Update::thunk(RE::PlayerCharacter* player, float delta) {

	func(player, delta);

	if (!player) {
		logger::warn("player is null cant check cell");
		return;
	}

	handlePendingMerges();

	// if cell is loaded lets wait 1 second before continueing. 
	if (globals::cellFullyLoaded.load() && !globals::secondAfterCellFullyLoaded.load()) {
		if (OneSecondPassed(globals::cellFullyLoadedTimerStart)) {
			globals::secondAfterCellFullyLoaded.store(true); 
		}
		return; 
	}

	//used to clean light props forced darkness to 0.0f for 1 second
	if (globals::secondAfterCellFullyLoaded.load()) {
		// clear so ref can be reprocessed again. for mods like dynamic candles
		std::lock_guard lock(globals::refsWithAttachedLightsMutex);
		globals::refsWithAttachedLights.clear();

		// clear so ref can be reprocessed again. for mods like dynamic candles
		std::lock_guard lock2(globals::mergedRefsMutex);
		globals::mergedRefs.clear();

		// idk if need to make this false again but anyway why not
		globals::cellFullyLoaded.store(false);
	}

	auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
	if (!ssNode) {
		logger::warn("ShadowSceneNode[0] is null!");
		return;
	}

	auto& ssRt = ssNode->GetRuntimeData();

	//auto playerPos = player->GetPosition(); 

	ApplyLightFlicker(ssRt.activeLights, delta);

	ApplyLightFlicker(ssRt.activeShadowLights, delta);

	// check stored bs shader properties have created there light list yet and tag their 
	//forced darkness field with a static # of a guesstamite of their light size. 
}

void PlayerCharacter_Update::Install()
{

	func = REL::Relocation<std::uintptr_t>(RE::PlayerCharacter::VTABLE[0])
		.write_vfunc(0xAD, thunk);
}