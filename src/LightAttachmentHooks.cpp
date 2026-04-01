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

	// this looks for refs, for performance, we only look up mod name of ref If its a light plugin, otherwise just ref id lookup
	if (auto* refCfgs = LightManager::findConfigsForRef(a_this, isInterior)) {
	
		static bool alreadyAttachedDebugMarker = false;

		for (const auto& cfg : *refCfgs) {

			auto cloneLight = LightManager::cloneNiPointLight(LightData::masterNiPointLight.light.get());

			if (!cloneLight) {
				logger::warn("Failed to clone NiPointLight for specific ref {:08X})", refFormID);
				return niAVObject;
			}

			if (!alreadyAttachedDebugMarker) {
				if (globals::enableDebugLightBulbs) LightManager::AttachDebugMarker(a_root, cloneLight);
				alreadyAttachedDebugMarker = true;
			}

			LightManager::attachLightUsingAttachPath(cfg, a_root, cloneLight, a_this->GetFormID());

			LightData::setNiPointLightDataFromCfg(cloneLight, cfg);

			cloneLight->name = "RL" + cfg.menuName;

			LightManager::attachNiPointLightToShadowSceneNode(cloneLight, cfg, a_this);
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

//yoinked this from PO3 Light Placer used only for torch light, could be used for weapons / armor
// im unable figure out how 1st person / 3rd person lights work. according to logging, in vanilla both are attachd at the 1st 
//attachlight node in the node tree of a torch. I think the game constantly update the first persons lights position as the player moves.
//would love to have full torch light control someday of torches but reusing vanilla light here is good enough for now.
void AddonNodes::thunk(
	RE::NiAVObject* a_clonedNode,
	RE::NiAVObject* a_node,
	std::int32_t a_slot,
	RE::TESObjectREFR* a_actor,
	RE::BSTSmartPointer<RE::BipedAnim>& a_bipedAnim)
{
    func(a_clonedNode, a_node, a_slot, a_actor, a_bipedAnim);
		 
	    // slot 9 == torch
	if (a_slot == 9 && globals::secondAfterCellFullyLoaded.load()) {

            SKSE::GetTaskInterface()->AddTask([a_actor]() {
                auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                if (!ssNode)
                    return;

                auto& rt = ssNode->GetRuntimeData();

                for (auto& lightEntry : rt.activeLights) {
                    if (!lightEntry)
                        continue;

                    auto* light = lightEntry->light.get();
                    if (!light || !light->parent || !light->parent->name.c_str())
                        continue;

					std::string parentName = light->parent->name.c_str(); 

					if (parentName != "AttachLight") {
						//logger::warn("users torch node tree does not contain object w name AttachLight, cant attach light");
						continue; 
					}	

					auto parent = light->parent->AsNode();
					if (!parent) {
						logger::warn("couldn’t cast torch as node will not apply light to torches");
						return;
					}
					
					std::string torchName = "torch";

					auto cell = a_actor->GetParentCell();

					if (!cell) {
						logger::warn("no cell cant determine if should use exterior or interior configs");
						return;
					}

					bool isInterior = cell->IsInteriorCell();

					auto cfgs = findConfigsForMeshPath(torchName, isInterior);
					if (cfgs.empty())
						continue;

					auto& cfg = cfgs[0];

					light->name = "RL" + torchName;
					light->unk138 = cfg.configID;

					LightData::setNiPointLightDataFromCfg(light, cfg);

					lightEntry->unk060 = 4; 

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