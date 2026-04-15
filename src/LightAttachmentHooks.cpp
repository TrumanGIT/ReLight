#include "LightAttachmentHooks.h"
#include "LightManager.h"
#include "Utility.h"

// ATTACH LIGHTS DURING LOAD3D() HOOK, ANY EARLIER AND LIGHTS SPAWN AT CELL ORIGIN BC WORLD POSITION DATA ISENT LOADED?

RE::NiAVObject* Load3D::thunk(RE::TESObjectREFR* a_this, bool a_backgroundLoading)
{

	//logger::info("load3D called");
	auto niAVObject = func(a_this, a_backgroundLoading);
	if (!niAVObject || !a_this) {
		//logger::warn("no ni node casted from niav object from load3d hook");
		return niAVObject;
	}

	RE::FormID refFormID = a_this->GetFormID();

	// ref already has a light placed, introduced to skip over refs that got a merged light
	{
		std::lock_guard lock(globals::refsWithAttachedLightsMutex);
		if (globals::refsWithAttachedLights.count(refFormID) > 0)
			return niAVObject;
	}
	{
		std::lock_guard lock(globals::mergedRefsMutex);
		if (globals::mergedRefs.count(refFormID) > 0)
			return niAVObject;
	}
	
	// calling asNode crashed on some dyndolod references for a user so netimmersive cast instead
	auto a_root = netimmerse_cast<RE::NiNode*>(niAVObject);
	if (!a_root) {
		return niAVObject;
	}

	auto cell = a_this->GetParentCell();

	if (!cell) {
		logger::warn("no cell cant determine if should use exterior or interior configs");
		return niAVObject;
	}

	bool isInterior = cell->IsInteriorCell();

	// this looks for refs
	if (auto* refCfgs = LightManager::findConfigsForRef(a_this, isInterior)) {

		bool alreadyAttachedDebugMarker = false;

		for (const auto& cfg : *refCfgs) {

			auto* light = LightManager::AttachLight(
				cfg,
				a_root,
				a_this,
				cfg.meshPath,
				refFormID,
				alreadyAttachedDebugMarker);

			if (!light) {
				logger::warn("AttachLight failed for ref {:08X} with mesh '{}'", refFormID, cfg.refFormIDAndModName);
			}
		
		}

		if (globals::removeFakeGlowOrbs) {

			auto node = niAVObject->AsNode();

			if (node) {
				glowOrbRemover(node);
			}
		}

		return niAVObject;
	}

	const auto baseObject = a_this->GetBaseObject();
	if (!baseObject) return niAVObject;

	const auto baseFormID = baseObject->GetFormID(); 

	const auto bm = baseObject->As<RE::TESModel>();
	if (!bm) return niAVObject;

	auto currentModel = std::string(bm->GetModel());

	auto meshName = extractMeshName(currentModel);

	//mutable
	toLower(meshName);

	// check file paths first, they will win over loose partial node name matches
	if (LightManager::processByFilePath(a_this, meshName, a_root, isInterior)) {
		globals::baseFormsWithAttachedLights.emplace(baseFormID);


		if (globals::removeFakeGlowOrbs) {

			auto node = niAVObject->AsNode();

			if (node) {
				glowOrbRemover(node);
			}
		}
		
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

//yoinked this from PO3 Light Placer used only for torch light, could be used for weapons / armor and spells like candle light
// im unable figure out how 1st person / 3rd person lights work. according to logging, in vanilla both are attachd at the 1st 
//attachlight node in the node tree of a torch. I think the game constantly update the first persons lights position as the player moves.

//TODO:: this is poorly implemented, possibly editing all torch sconces in the area as well when were only trying to target handheld torches
void AddonNodes::thunk(
	RE::NiAVObject* a_clonedNode,
	RE::NiAVObject* a_node,
	std::int32_t a_slot,
	RE::TESObjectREFR* a_actor,
	RE::BSTSmartPointer<RE::BipedAnim>& a_bipedAnim)
{
    func(a_clonedNode, a_node, a_slot, a_actor, a_bipedAnim);
		 
	    // slot 9 == torch or candle light ect, we wait 1 second after cell fully loaded or crash because of 1 hooks call site idk why 
	if (a_slot == 9 && globals::secondAfterCellFullyLoaded.load()) {

		// delay with add task or the light hasent appeared in the shadow scene node active light list yet
            SKSE::GetTaskInterface()->AddTask([]() {
                auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                if (!ssNode)
                    return;

                auto& rt = ssNode->GetRuntimeData();

				// try to find the light from the hooks arguments
                for (auto& lightEntry : rt.activeLights) {
                    if (!lightEntry)
                        continue;

                    auto* light = lightEntry->light.get();

                    if (!light || !light->parent || !light->parent->name.c_str())
                        continue;

					if (!light->parent->parent || !light->parent->parent->name.c_str())
						continue;

					std::string parentName = light->parent->name.c_str(); 

					std::string parentsParentName = light->parent->parent->name.c_str(); 

					// this is where the light would be in the torch or whatevers node tree
					if (parentName != "AttachLight") {
						//logger::warn("users torch node tree does not contain object w name AttachLight, cant attach light");
						continue; 
					}	

					// catches magic lights and torches and this excludes them in bslightingshaderhook as well (important)
					lightEntry->unk060 = 4;

					// this check prevents editing lights to things like magic spell candly light ect
					//tbh we should allow users to edit all lights one day so ill prolly remake this so users can edit spells too.
					if (!parentsParentName.contains("orch")) continue; 

					auto parent = light->parent->AsNode();
					if (!parent) {
						logger::warn("couldn’t cast torch as node will not apply light to torches");
						continue;
					}
					
					std::string torchName = "torch";

					auto cfgs = findConfigsForMeshPath(torchName, globals::currentCellIsInterior);
					if (cfgs.empty())
						continue;

					auto& cfg = cfgs[0];

					light->name = "RL" + torchName;
					light->unk138 = cfg.configID;

					// set scale to 1.0 I doubt user is using a scaled down size torch
					LightData::setNiPointLightDataFromCfg(light, cfg, 1.0);

					logger::debug("Applied torch light data"); 
                }
            });
    } 
}

void AddonNodes::Install()
{
	// this addres crashes must wait for a delay after game is loaded idk why
	std::array targets{
		std::make_pair(
			RELOCATION_ID(15501, 15678),       
			REL::VariantOffset{ 0xCBF, 0x617, 0} 
		), 

			std::make_pair(
			RELOCATION_ID(15524, 15701),
			REL::VariantOffset{0x193, 0x193, 0}
		), 
		
					std::make_pair(
			RELOCATION_ID(15526, 15703),
			REL::VariantOffset{0x1D7, 0x1D7, 0}
		)
	};

	for (auto& [id, offset] : targets) {
		REL::Relocation<std::uintptr_t> target(id, offset);
		auto& trampoline = SKSE::GetTrampoline();
		func = trampoline.write_call<5>(target.address(), thunk);
	}
}