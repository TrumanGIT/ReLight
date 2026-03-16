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
	 
	//is this cell fully laoded check that runs once enough to postpone until later? 
	if (globals::cellFullyLoaded) handlePendingMerges();

}

void PlayerCharacter_Update::Install()
{

	func = REL::Relocation<std::uintptr_t>(RE::PlayerCharacter::VTABLE[0])
		.write_vfunc(0xAD, thunk);
}