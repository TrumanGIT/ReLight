#include "flicker.h"
#include "LightManager.h"

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

	//setMaxLightDistance(ssRt.activeLights, playerPos, ssNode); 

//	setMaxLightDistance(ssRt.activeShadowLights, playerPos, ssNode);

	//logger::debug(" checking tri positions");


disableLightsNotInCamera(ssRt.activeLights, ssNode, worldCamera);

	disableLightsNotInCamera(ssRt.activeShadowLights, ssNode, worldCamera);

	ApplyLightFlicker(ssRt.activeLights, delta);

	ApplyLightFlicker(ssRt.activeShadowLights, delta);

	/*for (auto& light : ssRt.activeLights) {
		if (!light)
			continue;

		bool anyTriVisible = false;
		logger::info("Checking light at ({}, {}, {})", light->worldTranslate.x, light->worldTranslate.y, light->worldTranslate.z);

		for (auto& triShape : light->geomList) {
			if (!triShape)
				continue;

			auto& triPos = triShape->world.translate;
			logger::info("  triShape at ({}, {}, {})", triPos.x, triPos.y, triPos.z);
			rr
			// Get the bounding sphere radius from the triShape
			float radius = 0.0f;
			 trriShape->modelBound)
				radius = triShape->modelBound->radius;
			}
	
	}*/

}

void PlayerCharacter_Update::Install()
{

	func = REL::Relocation<std::uintptr_t>(RE::PlayerCharacter::VTABLE[0])
		.write_vfunc(0xAD, thunk);
}