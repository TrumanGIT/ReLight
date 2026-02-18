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

	auto worldSceneGraph = GetWorldSceneGraph();
	//logger::debug("worldSceneGraph loaded");
	auto worldCamera = ((RE::BSSceneGraph*)worldSceneGraph)->GetRuntimeData().camera.get();
	//logger::debug("worldCamera Loaded");

	auto& ssRt = ssNode->GetRuntimeData();

	auto playerPos = player->GetPosition(); 

	if (globals::maxLightDistanceEnabled) {
		disableLightsPastMaxDistance(ssRt.activeLights, playerPos, ssNode);

		disableLightsPastMaxDistance(ssRt.activeShadowLights, playerPos, ssNode);
	}

	if (globals::disableLightsNotInCameraEnabled) {
		disableLightsNotInCamera(ssRt.activeLights, ssNode, worldCamera, player);

		disableLightsNotInCamera(ssRt.activeShadowLights, ssNode, worldCamera, player);
	}

	ApplyLightFlicker(ssRt.activeLights, delta);

	ApplyLightFlicker(ssRt.activeShadowLights, delta);

}

void PlayerCharacter_Update::Install()
{

	func = REL::Relocation<std::uintptr_t>(RE::PlayerCharacter::VTABLE[0])
		.write_vfunc(0xAD, thunk);
}