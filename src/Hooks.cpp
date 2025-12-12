#include "Hooks.h"
#include "Functions.h"
#include "global.h"
#include <map>
#include <array>
#include <string>
#include <unordered_set>
#include "lightdata.h"

namespace Hooks {

	//Po3's
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
		const RE::TESFile* refOriginFile = ref->GetDescriptionOwnerFile();
		std::string modName = refOriginFile ? refOriginFile->fileName : "";

		toLower(modName);

		if (LightData::shouldDisableLight(light, ref, modName))
			return nullptr;

		// if whitelisted, change rgb values to whatever we want    
	   //doint want to change color of lights that change color based on time of day. 
		/*if (modName.find("window shadows ultimate") == std::string::npos)
		{
			light->data.color.red = red;
			light->data.color.green = green;
			light->data.color.blue = blue;
			
			//  logger::info("Changed color for light {:X} from {}", formID, modName);
		}*/

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

	RE::NiAVObject* Load3D::thunk(RE::TESObjectREFR* a_this, bool a_backgroundLoading)
	{

		if (!a_this) {
			logger::warn("Load3D called with null a_this");
			return func(a_this, a_backgroundLoading);
		}

		//logger::info("load3D called");
		auto niAVObject = func(a_this, a_backgroundLoading);
		if (!niAVObject) { 
			logger::warn("no ni node casted from niav object in load3d  with base editor id:{}", edid);
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
			logger::debug("Load3D() matched node name: {}", nodeName);
			if (isExclude(nodeName, a_root)) return niAVObject;
	
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
	
		    auto nodePtr = getNextNodeFromBank(match);
			if (nodePtr) {
				logger::debug("next node retrieved successfully ", match);
				a_root->AttachChild(nodePtr.get());

				//    logger::info("attached light to keyword mesh {}", nodeName);
				LightConfig cfg = findConfigForNode(nodeName);
				LightData::attachNiPointLightToShadowSceneNode(getNextNodeFromBank(match).get(), cfg);
			}
		}

	//	dummyHandler(a_root, nodeName);

		return niAVObject;
	}

	void Load3D::Install()
	{
		logger::info("Installing Load3D hook...");

		func = REL::Relocation<std::uintptr_t>(RE::TESObjectREFR::VTABLE[0])
			.write_vfunc(idx, thunk);

		logger::info("Hooked TESObjectREFR::Load3D");
	}

	void Install() {
		SKSE::AllocTrampoline(1 << 8);
		TESObjectLIGH_GenDynamic::Install();
		Load3D::Install();
	}
}