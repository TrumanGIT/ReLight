#include "LightAttachmentHooks.h"
#include "LightManager.h"
#include "Utility.h"


// ATTACH LIGHTS DURING LOAD3D() HOOK, ANY EARLIER AND LIGHTS SPAWN AT CELL ORIGIN BC WORLD POSITION DATA ISENT LOADED?

RE::NiAVObject* Load3D::thunk(RE::TESObjectREFR* a_this, bool a_backgroundLoading)
{
	if (!a_this || a_backgroundLoading == false) {
		//	logger::debug("Load3D called with null a_this or bg loading = true (light were trying to reinitialize) skipping light attachment");
		return func(a_this, a_backgroundLoading);
	}

	// ref already has a light placed, introduced to skip over refs that got a merged light
	if (globals::refsWithAttachedLights.count(a_this) > 0) {
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

	if (LightManager::processByFilePath(a_this, a_root)) return niAVObject;

	// grab name of NiNode (usually 1:1 with mesh names)

	// some nodes have 2 config names in their nodename. for example we need to prioritize candlechangdelier01 to use chandelier lights over candle lights.
	const RE::BSFixedString nodeNameMatch = findPriorityMatch(a_root->name);

	if (!nodeNameMatch.empty()) {
		if (isExclude(a_root->name, a_root)) return niAVObject;

		LightManager::processByNodeName(a_root, nodeNameMatch, a_this);
		return  niAVObject;
	}

	if (LightManager::dummyHandler(a_this, a_root->name, a_root)) {
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
// im unable to remove the ni light from the torch for some rac
void AddonNodes::thunk(
    RE::NiAVObject* a_clonedNode,
    RE::NiAVObject* a_node,
    std::int32_t a_slot,
    RE::TESObjectREFR* a_actor,
    RE::BSTSmartPointer<RE::BipedAnim>& a_bipedAnim)
{

    func(a_clonedNode, a_node, a_slot, a_actor, a_bipedAnim);

        if (a_slot == 9) {

            SKSE::GetTaskInterface()->AddTask([=]() {
                auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                if (!ssNode)
                    return;

                auto& rt = ssNode->GetRuntimeData();

                for (auto& lightEntry : rt.activeLights) {
                    if (!lightEntry)
                        continue;

                    auto* light = lightEntry->light.get();
                    if (!light || !light->parent)
                        continue;

					if (light->parent->name != "AttachLight") {
						logger::warn("users torch node tree does not contain object w name AttachLight, cant attach light");
						continue; 
					}	

					auto parent = light->parent->AsNode();
					if (!parent) {
						logger::warn("couldn’t cast torch as node will not apply light to torches");
						return;
					}

					std::string torchName = "torch";

					auto cfgs = findConfigsForNode(torchName);
					if (cfgs.empty())
						continue;

					auto& cfg = cfgs[0];

					light->name = "RL" + torchName;
					light->unk138 = cfg.configID;

					RE::FormID formID = 0x0;
					LightData::setNiPointLightDataFromCfg(formID, light, cfg);

					logger::info("Applied torch config to {}", light->name);
                }
            });
        }
}


  void AddonNodes::Install()
{
	REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(15527, 15704) };
	hook_function_prologue<AddonNodes, 5>(target.address());

	logger::info("Hooked BipedAnim::AddAddonNodes");
}