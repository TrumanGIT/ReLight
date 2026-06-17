#include "LightAttachmentHooks.h"

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

	// skip harvested plants
	if (a_this->formFlags & (1 << 13)) {
		logger::debug("skip attaching light to harvested plant");
		return niAVObject;
	}

	const auto baseObject = a_this->GetBaseObject();
	if (!baseObject) return niAVObject;

	const auto baseFormID = baseObject->GetFormID();

	// this looks for refs
	if (auto* refCfgs = LightManager::findConfigsForRef(a_this, isInterior)) {

		bool alreadyAttachedDebugMarker = false;

		for (const auto& cfg : *refCfgs) {

			auto* light = LightManager::AttachLight(
				cfg,
				a_root,
				a_this,
				cfg.menuName,
				refFormID,
				alreadyAttachedDebugMarker);

			if (!light) {
				logger::warn("AttachLight failed for ref {:08X} with light '{}'", refFormID, cfg.menuName);
			}

			globals::baseFormsWithAttachedLights.emplace(baseFormID);
		}

		if (globals::removeFakeGlowOrbs) {
				glowOrbRemover(a_root);
		}
		
		return niAVObject;
	}

	// this looks for base
	if (auto* baseCfgs = LightManager::findConfigsForBase(baseFormID, isInterior)) {

		bool alreadyAttachedDebugMarker = false;

		for (const auto& cfg : *baseCfgs) {

			auto* light = LightManager::AttachLight(
				cfg,
				a_root,
				a_this,
				cfg.menuName,
				refFormID,
				alreadyAttachedDebugMarker);

			if (!light) {
				logger::warn("AttachLight failed for ref {:08X} with light '{}'", refFormID, cfg.menuName);
			}

			globals::baseFormsWithAttachedLights.emplace(baseFormID);

		}

		if (globals::removeFakeGlowOrbs) {
			glowOrbRemover(a_root);
		}

		return niAVObject;
	}
	
	const auto bm = baseObject->As<RE::TESModel>();
	if (!bm) return niAVObject;

	auto currentModel = std::string(bm->GetModel());

	//turn mesh name from //statics//whiterun//objects//fires.nif -> fires
	auto meshName = extractMeshName(currentModel);

	//mutable
	toLower(meshName);

	//async task to handle scritped fires, skips if animations are shut off
	if (LightManager::HandleScriptedFires(a_this, baseFormID, meshName, isInterior)) {
		return niAVObject;
	}

	if (LightManager::processByFilePath(a_this, meshName, a_root, isInterior)) {
		globals::baseFormsWithAttachedLights.emplace(baseFormID);

			if (globals::removeFakeGlowOrbs) {
				glowOrbRemover(a_root);
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

//yoinked this from PO3 Light Placer used only for torch light, could be used for weapons / armor 
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
		 
	if (a_slot == 9) {

		logger::debug("cloned node = {} a_node = {}", a_clonedNode->name.c_str(), a_node->name.c_str());

		// delay with add task or the light hasent appeared in the shadow scene node active light list yet

		auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
		if (!ssNode || !a_node)
			return;

		auto* torchFire = a_node->GetObjectByName("TorchFire");
		if (!torchFire)
			return;

		auto* attachLight = a_node->GetObjectByName("AttachLight");
		if (!attachLight)
			return;

		// deal with it a second later otherwise light isent ready yet
		globals::torchLightAttachNodes.emplace_back(attachLight);

		logger::debug("torchfire + attachlight nodes found sending to torch queue");
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

bool Activate::thunk(
	RE::TESObjectACTI* a_this,
	RE::TESObjectREFR* a_targetRef,
	RE::TESObjectREFR* a_activatorRef,
	std::uint8_t a_arg3,
	RE::TESBoundObject* a_object,
	std::int32_t a_targetCount)
{

	bool result = func(a_this, a_targetRef, a_activatorRef, a_arg3, a_object, a_targetCount);

	if (!a_targetRef ||! a_this) {
	//	logger::info("a_targetRef is nullptr");
		return result;
	}

	LightManager::HandleSkyHavenTempleScriptedFires(a_targetRef); 


	LightManager::HandleDLC1VCDungeonScriptedFires(a_targetRef); 

	return result;
}
void Activate::Install()
{
	func = REL::Relocation<std::uintptr_t>(RE::TESObjectACTI::VTABLE[0])
		.write_vfunc(idx, thunk);
	logger::info("Hooked TESObjectACTI::Activate");
}
