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
	
		//const float baseDelta = RE::BSTimer::GetSingleton()->realTimeDelta;
		const float deltaTime = RE::BSTimer::GetSingleton()->realTimeDelta;

		//static float 60FPSdeltaTime = 0.016f; // this is relevent to how many frames per second the hook is called, default 60fps

		auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
		if (!ssNode) {
			logger::warn("ShadowSceneNode[0] is null!");
			return;
		}

		auto& rt = ssNode->GetRuntimeData();

		for (auto& light : rt.activeLights) {
			if (!light) continue;

			//Check if Relight light by prefix "RL"
			const char* name = light->light->name.c_str();
			if (!name || name[0] != 'R' || name[1] != 'L')
				continue;

			//`	logger::debug("PlayerUpdate: Relight light found {}", lightName); 

			auto& data = light->light->GetLightRuntimeData();

			if (auto* lightRuntimeData = Overlay::Get(light->light.get())) {

				const auto r = lightRuntimeData->getRandomFloat(-lightRuntimeData->speedRandomness, lightRuntimeData->speedRandomness);

				lightRuntimeData->time += deltaTime * (1 - r) * std::numbers::pi_v<float>;
				data.fade = lightRuntimeData->startingFade + std::sin(lightRuntimeData->time * lightRuntimeData->flickersPerSecond) * lightRuntimeData->flickerIntensity;
			}
			else logger::warn("no isl overlay for light: ");
		}
	 }
	
	 void PlayerCharacter_Update::Install()
	{

		func = REL::Relocation<std::uintptr_t>(RE::PlayerCharacter::VTABLE[0])
			.write_vfunc(0xAD, thunk);
	}

	static bool firstUpdate = true;


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

		if (LightData::excludeLightEditorID(light))
			return func(light, ref, node, forceDynamic, useLightRadius, affectRequesterOnly);

		// get the name of the mod owning the light
	//	const RE::TESFile* refOriginFile = ref->GetDescriptionOwnerFile();
		//std::string modName = refOriginFile ? refOriginFile->fileName : "";

		//toLower(modName);

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

		auto ui = RE::UI::GetSingleton();

		if (ui && ui->IsMenuOpen("InventoryMenu")) {
			//logger::info("Inventory menu is open, skipping PostCreate processing"); // do we even need that? 
			return niAVObject;
		}

		// grab name of NiNode (usually 1:1 with mesh names)
		std::string nodeName = a_root->name.c_str();
		toLower(nodeName);

		// some nodes have 2 config names in their nodename. for example we need to prioritize candlechangdelier01 to use chandelier lights over candle lights.
		auto match = findPriorityMatch(nodeName);

		if (!match.empty() /* || nodeName.find("nortmphallbgc") != std::string::npos || nodeName.find("norcathallsm") != std::string::npos || nodeName.find("scene") != std::string::npos*/) {
		//	logger::debug("Load3D() matched node name: {}", nodeName);

			if (isExclude(nodeName, a_root)) return niAVObject;

			const auto baseObject = a_this->GetBaseObject(); 

			const auto baseFormID = baseObject ? baseObject->GetFormID() : 0;

			if (baseFormID != 0) {
				baseFormsWithAttachedLights.emplace(baseFormID);
				logger::debug("node: {} with baseFormID: {}  emplaced in set", nodeName, baseFormID);
			}
				
			//TODO:: Reimplement, no nifpath in args of hook but can still prolly pull mod path
			 // if (handleSceneRoot(a_nifPath, a_root, nodeName))
			  //    return niAVObject;

			if (removeFakeGlowOrbs)
				glowOrbRemover(a_root);

		//	if (TorchHandler(nodeName, a_root))
			//	return niAVObject;

			//TO DO:: need a new way to handle nordic meshes bc we cant iterate through a nif template like with mlo2
		   /* if (applyCorrectNordicHallTemplate(nodeName, a_root))
				return func(a_this, a_args, a_nifPath, a_root, a_typeOut);*/
		
	       LightConfig cfg = findConfigForNode(match);

		   auto cloneLight =  cloneNiPointLight(masterNiPointLight.get());

		   if (!cloneLight) {
			   logger::warn("Failed to clone NiPointLight for node '{}')", nodeName);
			   return niAVObject;
		   }

			//auto niPointer = RE::NiPointer<RE::NiLight>(niLight);

			LightData::setNiPointLightDataFromCfg(cloneLight, cfg, cfg.nodeName);

			/// TODO:: if not in priority list in ini file, this causes name to be RL only need to fix that
			cloneLight->name = "RL" + cfg.nodeName;

			a_root->AttachChild(cloneLight); 

			LightData::attachNiPointLightToShadowSceneNode(cloneLight, cfg);
			
			//logger::debug("LightName: {}, created ", match);	
		}

	//	dummyHandler(a_root, nodeName);

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