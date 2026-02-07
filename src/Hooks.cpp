#include <map>
#include <array>
#include <string>
#include <numbers>
#include <cmath>
#include "Hooks.h"
#include "Functions.h"
#include "global.h"
#include <unordered_set>
#include "lightdata.h"

namespace Hooks {

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

		ApplyLightFlicker(ssRt.activeLights, delta);
			
		ApplyLightFlicker(ssRt.activeShadowLights, delta);
	 
	}
	
	 void PlayerCharacter_Update::Install()
	{

		func = REL::Relocation<std::uintptr_t>(RE::PlayerCharacter::VTABLE[0])
			.write_vfunc(0xAD, thunk);
	}


	//Po3's hook (disable vanilla lights for a clean base to start with) 
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


		if (LightData::shouldDisableLight(light, ref))
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

	// this is when we attach lights because world position data is loaded at this point and 
	//your bs lights will reflect your ni point light position. any earlier and they spwan at cell origin
	RE::NiAVObject* Load3D::thunk(RE::TESObjectREFR* a_this, bool a_backgroundLoading)
	{
		if (!a_this) {
			logger::warn("Load3D called with null a_this");
			return func(a_this, a_backgroundLoading);
		}

		//logger::info("load3D called");
		auto niAVObject = func(a_this, a_backgroundLoading);
		if (!niAVObject) { 
			logger::warn("no ni node casted from niav object from load3d hook");
			return niAVObject;
		}

		//helps filter out a few things we dont want to touch (fog, mist)
		auto a_root = niAVObject->AsNode();
		if (!a_root) {
			logger::warn("no ni node casted from niav object in load3d");
			return niAVObject;
		}

		if (processByFilePath(a_this, a_root)) return niAVObject;

		// grab name of NiNode (usually 1:1 with mesh names)
	
		// some nodes have 2 config names in their nodename. for example we need to prioritize candlechangdelier01 to use chandelier lights over candle lights.
		const RE::BSFixedString nodeNameMatch = findPriorityMatch(a_root->name);

		if (!nodeNameMatch.empty()) {
			if (isExclude(a_root->name, a_root)) return niAVObject;

			processByNodeName(a_root, nodeNameMatch, a_this);
			return  niAVObject;
		}

		if (dummyHandler(a_this, a_root->name, a_root)) {
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

	void Install() {
		SKSE::AllocTrampoline(1 << 8);
		TESObjectLIGH_GenDynamic::Install();
		Load3D::Install();
		PlayerCharacter_Update::Install();
	}
}