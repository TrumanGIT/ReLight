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

	//skips fires with base ids below with animations off 
	// 1. Sky Haven chain activated fires
	// 2. Castle Volkihar fires that turn on
	if (!LightManager::IsAnimationsOn(a_this, baseFormID)) {
		return niAVObject;
	}

	// this looks for refs
	if (auto* refCfgs = LightData::findConfigsByFormID(refFormID, isInterior, false)) {

		bool alreadyAttachedDebugMarker = false;

		for (const auto& cfg : *refCfgs) {

			if (cfg.isPluginLight) return niAVObject;

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
	if (auto* baseCfgs = LightData::findConfigsByFormID(baseFormID, isInterior, true)) {

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