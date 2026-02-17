#include "LightManager.h"
#include "Utility.h"
#include "config.hpp"


// ATTACH LIGHTS DURING LOAD3D() HOOK, ANY EARLIER AND LIGHTS SPAWN AT CELL ORIGIN BC WORLD POSITION DATA ISENT LAODED?

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


//ATTACH LIGHTS AT CORRECT MESH INDEX, USEFULL FOR TORCHES WHERE LIGHT MUST BE INSERTED TO SPECIFIC SPOT
void LightManager::attachLightUsingAttachPath(
	const LightConfig& cfg,
	RE::NiNode* root,
	RE::NiPointLight* light, RE::FormID& refFormID)
{
	if (!root || !light) {
		logger::warn("attachLightUsingAttachPath: null root or light");
		return;
	}

	RE::NiAVObject* current = root;

	for (int index : cfg.attachPath) {
		auto* node = current->AsNode();
		if (!node) {
			logger::warn("attachLightUsingAttachPath: object is not a NiNode");
			return;
		}

		auto& children = node->GetChildren();
		if (index < 0 || index >= children.size() || !children[index]) {
			logger::warn("attachLightUsingAttachPath: index {} out of bounds", index);
			return;
		}

		current = children[index].get();
	}

	auto* finalNode = current->AsNode();
	if (!finalNode) {
		logger::warn("attachLightUsingAttachPath: final target is not a NiNode");
		return;
	}

	auto finalNodeName = finalNode->name.c_str();

	logger::debug("attached light to node {} on ref {}", finalNodeName, refFormID);

	finalNode->AttachChild(light);
}

// ATTACH NI POINT LIGHT TO SHADOW SCENE NODE TO GET BS LIGHT IN RETURN. BS LIGHT IS THE LIGHT YOU VISUALLY SEE RENDERED IN GAME
//BS LIGHT READS ITS NI POINT LIGHT DATA EVERY FRAME
void LightManager::attachNiPointLightToShadowSceneNode(RE::NiLight* niPointLight, const LightConfig& cfg, RE::FormID& refFormID) {

	//logger::info("attempting to create NiPointLight BSlight and attach to ShadowSceneNode");

	if (!niPointLight) {
		logger::error("createShadowSceneNode: niPointLight is null");
		return;
	}

	RE::ShadowSceneNode::LIGHT_CREATE_PARAMS params = LightData::makeLightParams(cfg);

	logger::debug("Light paramaters for light {} created for ref {} ", niPointLight->name, refFormID);

	LightData::printLightParams(params);

	auto* shadowSceneNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];

	if (!shadowSceneNode) {
		logger::warn("no shadow scene node to grab in (createShadowSceneNode()");
		return;
	}

	RE::BSLight* BsLight = shadowSceneNode->AddLight(niPointLight, params);


	if (!BsLight) {
		logger::info("no BSLight created in (createShadowSceneNode() for {}", niPointLight->name);
		return;
	}
}

//TODO:: doesent handle multi lights in a single config
 bool LightManager::processByFilePath(RE::TESObjectREFR* a_this, RE::NiNode* a_root) {

	if (LightData::meshPathToJsonCfg.empty()) return false;

	const auto baseObject = a_this->GetBaseObject();

	if (!baseObject) return true;

	const auto bm = baseObject->As<RE::TESModel>();
	if (!bm) return true;

	auto currentModel = bm->GetModel();

	for (const auto& [meshPath, cfg] : LightData::meshPathToJsonCfg) {

		if (meshPath != currentModel) continue;

		const auto baseFormID = baseObject->GetFormID();

	  auto refFormID = a_this->GetFormID(); 

		globals::baseFormsWithAttachedLights.emplace(baseFormID);

		logger::debug("file path match found: {}", currentModel);

		auto ui = RE::UI::GetSingleton();

		if (ui && ui->IsMenuOpen("InventoryMenu")) {
			//logger::info("Inventory menu is open, skipping PostCreate processing"); // do we even need that? 
			return true;
		}

		auto cloneLight = cloneNiPointLight(PointLight::getMasterPointLight().node.get());

		if (!cloneLight) {
			logger::warn("Failed to clone NiPointLight for mesh '{}')", currentModel);
			continue;
		}

		LightData::setNiPointLightDataFromCfg(refFormID,cloneLight, cfg);

		/// TODO:: if not in priority list in ini file, this causes name to be RL only need to fix that
		std::string temp = "RL" + std::string(cfg.nodeName.c_str());
		cloneLight->name = temp.c_str();

		LightManager::attachLightUsingAttachPath(cfg, a_root, cloneLight, refFormID);

		LightManager::attachNiPointLightToShadowSceneNode(cloneLight, cfg, refFormID);
		return true;

	}

	return false;
}

 //TODO:: doesent handle multi lights in a single config
// some nodes are called dummy this is to take care of them.
 bool LightManager::dummyHandler(RE::TESObjectREFR* a_this, const RE::BSFixedString& nodeName, RE::NiNode* a_root)
{

	if (!nodeName.contains("dummy")) return false;

	logger::debug("dummy found");

	static const std::unordered_map<std::string, std::string> dummyMeshPaths = {
	{ "Clutter\\Ruins\\RuinsFloorCandleLampMidOn.nif", "ruinsfloorcandlelampmidon" },
	{ "Clutter\\Ruins\\RuinsFloorCandleLampMidOn02.nif", "ruinsfloorcandlelampmidon" },
	{ "Clutter\\Ruins\\RuinsFloorCandleLampSmOn.nif", "ruinsfloorcandlelampsmon" },
	{ "Clutter\\Ruins\\RuinsFloorCandleLampSmOn02.nif", "ruinsfloorcandlelampsmon" },

	{ "Clutter\\Imperial\\ImpChandellierCandle01.nif", "chandel" },
	{ "Clutter\\Imperial\\ImpChandellierCandle01USKP.nif", "chandel" },

	{ "Clutter\\Common\\CandleLanternwithCandle01.nif", "candle" },

	{ "DynDOLOD\\LOD\\Clutter\\CandleLanternHandleDown_DynDOLOD_LOD.nif", "candle" },
	{ "DynDOLOD\\LOD\\Clutter\\CandleLanternwithCandle01_DynDOLOD_LOD.nif", "candle" },

	{ "DynDOLOD\\LOD\\Clutter\\ImpChandellierCandle01_DynDOLOD_LOD.nif", "chandel" },

	{ "DynDOLOD\\LOD\\Clutter\\RuinsFloorCandleLampMidOn_DynDOLOD_LOD.nif", "ruinsfloorcandlelampmidon" },
	{ "DynDOLOD\\LOD\\Clutter\\RuinsFloorCandleLampMidOn02_DynDOLOD_LOD.nif", "ruinsfloorcandlelampmidon" },
	{ "DynDOLOD\\LOD\\Clutter\\RuinsFloorCandleLampSmOn_DynDOLOD_LOD.nif", "ruinsfloorcandlelampsmon" },
	{ "DynDOLOD\\LOD\\Clutter\\RuinsFloorCandleLampSmOn02_DynDOLOD_LOD.nif", "ruinsfloorcandlelampsmon" },
	};

	auto baseObject = a_this->GetBaseObject();

	if (!baseObject) return true;

	const auto baseFormID = baseObject->GetFormID();

	const auto bm = baseObject->As<RE::TESModel>();
	if (!bm) return true;

	const auto currentModel = bm->GetModel();

	logger::debug("dummy found, Model = {}", currentModel);

	auto it = dummyMeshPaths.find(currentModel);
	if (it != dummyMeshPaths.end()) {

		auto refFormID = a_this->GetFormID();

		globals::baseFormsWithAttachedLights.emplace(baseFormID);
		
		 std::string match = it->second;

		auto cfg = findConfigsForNode(match)[0];

		auto cloneLight = cloneNiPointLight(PointLight::getMasterPointLight().node.get());

		if (!cloneLight) {
			logger::warn("Failed to clone NiPointLight for node '{}')", nodeName);
			return true;
		}

		LightData::setNiPointLightDataFromCfg(refFormID, cloneLight, cfg);

		cloneLight->name = "RL" + cfg.nodeName;

		LightManager::attachLightUsingAttachPath(cfg, a_root, cloneLight, refFormID);

		LightManager::attachNiPointLightToShadowSceneNode(cloneLight, cfg, refFormID);

		logger::debug("dummy node {} with model {} found for ref {} and got light from config: {}", nodeName.c_str(), currentModel, a_this->GetFormID(), cfg.nodeName );
		return true;
	}

	// already returned early if not a dummy, therefor might as well skip this object as it wouldent get light anyway
	return true;
}


 void LightManager::processByNodeName(RE::NiNode* a_root, const RE::BSFixedString& match, RE::TESObjectREFR* a_this) {

	// matched name
	std::string matchStr = match.c_str();

	auto refFormID = a_this->GetFormID(); 

	auto ui = RE::UI::GetSingleton();

	if (ui && ui->IsMenuOpen("InventoryMenu")) {
		//logger::info("Inventory menu is open, skipping PostCreate processing"); // do we even need that? 
		return;
	}

	const auto baseObject = a_this->GetBaseObject();

	const auto baseFormID = baseObject ? baseObject->GetFormID() : 0;

	if (baseFormID != 0) {
		globals::baseFormsWithAttachedLights.emplace(baseFormID);
		logger::debug(" processing ref {} with node name: {} with baseFormID: {} emplaced in set",refFormID, matchStr, baseFormID);
	}

	if (globals::removeFakeGlowOrbs)
		glowOrbRemover(a_root);

	auto cfgs = findConfigsForNode(matchStr);

	for (const auto& cfg : cfgs) {
		auto cloneLight = cloneNiPointLight(PointLight::getMasterPointLight().node.get());

		if (!cloneLight) {
			logger::warn("Failed to clone NiPointLight for node '{}')", matchStr);
			continue;
		}

		LightData::setNiPointLightDataFromCfg( refFormID, cloneLight, cfg);

		cloneLight->name = "RL" + cfg.nodeName;

		if (globals::enableLightMerging) {
			LightManager::attachOrMergeLight(a_this, refFormID, cloneLight, cfg, a_root);
		}
		else {
			LightManager::attachLightUsingAttachPath(cfg, a_root, cloneLight, refFormID);
		}

		LightManager::attachNiPointLightToShadowSceneNode(cloneLight, cfg, refFormID);

		//logger::info("attached {} light to {} ", cfg.nodeName, a_this->GetFormID());
	}
}

//USED TO REINITIALIZE LIGHTS CLEANED BY THE ENGINE. USUALLY WHEN GOING FROM INTERIOR TO EXTERIOR, BACK TO INTERIOR AND VICE VERSA
RE::BSEventNotifyControl LightManager::ProcessEvent(const RE::BGSActorCellEvent* event,
	RE::BSTEventSource<RE::BGSActorCellEvent>*) {

	if (!event || event->flags == RE::BGSActorCellEvent::CellFlag::kLeave) {
		return RE::BSEventNotifyControl::kContinue;
	}

	auto player = RE::PlayerCharacter::GetSingleton();

	if (!player) return RE::BSEventNotifyControl::kContinue;

	static bool s_firstCellEvent = true;

	//logger::debug("cell event fired for player");

	auto cell = RE::TESForm::LookupByID<RE::TESObjectCELL>(event->cellID);
	if (!cell) {
		return RE::BSEventNotifyControl::kContinue;
	}

	//TODO Change to use global
	static int fLODFadeOutMultObjects;

	if (s_firstCellEvent) {
		s_firstCellEvent = false;
		globals::lastCellWasInterior = cell->IsInteriorCell();
		return RE::BSEventNotifyControl::kContinue;

		getObjectFadeMult(fLODFadeOutMultObjects);

		logger::debug("Users object fade ini setting = {}", fLODFadeOutMultObjects);

	}

	const bool currentCellIsInterior = cell->IsInteriorCell();

	if (globals::lastCellWasInterior != currentCellIsInterior) {
		
		LightManager::reinitializeLightsWithinRange(player); 
	 
	}

	globals::lastCellWasInterior = currentCellIsInterior;

	return RE::BSEventNotifyControl::kContinue;
}

void LightManager::registerEventSink()
{
	if (auto* player = RE::PlayerCharacter::GetSingleton()) {
		player->AsBGSActorCellEventSource()->AddEventSink(LightManager::GetSingleton());
		logger::info("BGSActorCellEvent sink registered");
	}
}

// TOODO:: put this in the event sink above
void LightManager::reinitializeLightsWithinRange(RE::PlayerCharacter* player) {

	logger::debug("reinitializing lights within range");

	// clear this lights list that has merged lights placed into it so tehey can get lights attached again.
	globals::refsWithAttachedLights.clear();

	auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
	if (!ssNode) {
		logger::warn("ShadowSceneNode[0] is null cant reinitialize lights");
		return;
	}
	auto& rt = ssNode->GetRuntimeData();

	//TODO Change to use global
	static int fLODFadeOutMultObjects;

	getObjectFadeMult(fLODFadeOutMultObjects);
	
	RE::TES::GetSingleton()->ForEachReferenceInRange(player, fLODFadeOutMultObjects, [](RE::TESObjectREFR* ref) {

		if (!ref) return RE::BSContainer::ForEachResult::kContinue;

		const auto baseObject = ref->GetBaseObject();

		auto baseFormID = baseObject ? baseObject->GetFormID() : 0;

		if (baseFormID == 0) return RE::BSContainer::ForEachResult::kContinue;

		for (const auto& formID : globals::baseFormsWithAttachedLights) {

			//logger::debug("Tried to match base form id: {} against: {}", baseFormID, formID);

			if (baseFormID == formID) {
				logger::debug("baseForm ref that needs reinitializing found");

				RE::ObjectRefHandle handle(ref);
				SKSE::GetTaskInterface()->AddTask([handle]() {
					if (auto ref = handle.get()) {
						auto root = ref->Load3D(false);

						if (!root) return;

						auto bsFadeNode = root->AsNode();

						if (!bsFadeNode) return;

						//surf children for light
						for (auto& child : bsFadeNode->GetChildren()) {
							if (!child) continue;

							// exclude non relight lights
							const char* name = child->name.c_str();
							if (!name || name[0] != 'R' || name[1] != 'L')
								continue;

							RE::NiPointLight* light = netimmerse_cast<RE::NiPointLight*>(child.get());

							if (!light) continue;

							auto it = LightData::configIDToJsonCfg.find(light->GetLightRuntimeData().unk138);

							if (it == LightData::configIDToJsonCfg.end()) {
								logger::warn("attempted to reinitialize light but its config ID wasent found");
								continue;
							}

							const auto& config = it->second;

							// shadow lights are handled differently then non shadow lights
							if (config.shadowLight) {

								auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
								if (!ssNode) {
									logger::warn("ShadowSceneNode[0] is null!");
									continue;
								}
								bool bsLightExists = false;

								for (auto bsLight : ssNode->activeShadowLights) {

									bsLight->worldTranslate;

									if (bsLight->light.get() == light) {
										logger::info("shadow light {} with ID {} exists already for ref: {} skipping reinitialization", light->name, static_cast<void*>(light), ref->GetFormID());
										bsLightExists = true;
										break;
									}
								}

								if (!bsLightExists) {
									logger::info("reintiializing shadow light {} for ref {} ", light->name, ref->GetFormID());

									auto p = LightData::makeLightParams(config);
									ssNode->AddLight(light, p);
								}

							}
							else {
								ref->Disable();
								ref->Enable(false);
								logger::info("non shadow light: {} reinitialized for ref {}", light->name, ref->GetFormID());
							}
						}

					}
				});
			}
		}
	});
}


//used to merge a light with same ref base object within a set distance to help prevent flickering. 
void LightManager::attachOrMergeLight(RE::TESObjectREFR* a_this, RE::FormID& refFormID,
	RE::NiPointLight* childLight, const LightConfig& cfg, RE::NiNode* a_root)
{
	if (!a_this || !childLight) return;

	std::vector<RE::TESObjectREFR*> pendingMerge;
	int potentialMergeCount = 0;
	RE::FormID a_thisBaseID = a_this->GetBaseObject() ? a_this->GetBaseObject()->GetFormID() : 0;

	// Find nearby refs with the same base object that haven't been merged yet
	RE::TES::GetSingleton()->ForEachReferenceInRange(a_this, globals::lightMergeDistance, [&](RE::TESObjectREFR* ref) {
		if (ref == a_this) return RE::BSContainer::ForEachResult::kContinue;
		if (globals::refsWithAttachedLights.count(ref) == 0) {
			auto baseObj = ref->GetBaseObject();
			if (baseObj && baseObj->GetFormID() == a_thisBaseID) {
				pendingMerge.push_back(ref);
				potentialMergeCount++;
			}
		}
		return RE::BSContainer::ForEachResult::kContinue;
		});

	switch (potentialMergeCount) {

		//1 merge found so place light in between 2 refs. 
	case 1: {

		LightManager::attachLightUsingAttachPath(cfg, a_root, childLight, refFormID);

		auto otherRef = pendingMerge[0];

		// Get world positions of the references we want the lgiht to go in between
		RE::NiPoint3 refAWorldPos = a_this->GetPosition();
		RE::NiPoint3 refBWorldPos = otherRef->GetPosition();

		// Compute midpoint in world space (X/Y only)
		RE::NiPoint3 worldMid{};
		worldMid.x = (refAWorldPos.x + refBWorldPos.x) * 0.5f;
		worldMid.y = (refAWorldPos.y + refBWorldPos.y) * 0.5f;
		// Preserve original Z as offset
		worldMid.z = refAWorldPos.z;

		// Save original local Z for the child
		float originalLocalZ = childLight->local.translate.z;

		// Convert world midpoint to local space of parent
		RE::NiTransform parentWorldTransform = a_root->world;
		RE::NiTransform invTransform = parentWorldTransform.Invert();
		RE::NiPoint3 localMid = invTransform * worldMid;

		// Add original Z offset
		localMid.z += originalLocalZ;

		// Logging before change
		logger::debug(
			"BEFORE Light {} merge for ref{} at {} and ref {} at {}. "
			"Desired world XY = ({}, {}), original Z = {}. "
			"Local translate = {}",
			childLight->name.c_str(),
			a_this->GetFormID(), refAWorldPos,
			otherRef->GetFormID(), refBWorldPos,
			worldMid.x, worldMid.y, originalLocalZ,
			childLight->local.translate
		);

		// Apply local position
		childLight->local.translate = localMid;

		// Optionally double fade
		childLight->fade *= 2.0f;

		// Force parent update to reflect changes in world space
		if (auto* parent = childLight->parent) {
			RE::NiUpdateData updateData{};
			updateData.time = 0.0f;
			updateData.flags = RE::NiUpdateData::Flag::kDirty;
			parent->UpdateTransformAndBounds(updateData);
		}

		// Track references with attached lights
		globals::refsWithAttachedLights.insert(a_this);
		globals::refsWithAttachedLights.insert(otherRef);

		// Logging after change
		logger::debug(
			"AFTER  Light {} merge for ref{} at {} and ref {} at {}. "
			"Local translate set = {}, world position approx = {}",
			childLight->name.c_str(),
			a_this->GetFormID(), refAWorldPos,
			otherRef->GetFormID(), refBWorldPos,
			childLight->local.translate,
			a_root->world.translate + childLight->local.translate // approximate
		);

		break;
	}
		  // find the middle of 3 refs and place light in the middle
	case 2: {
		LightManager::attachLightUsingAttachPath(cfg, a_root, childLight, refFormID);

		std::vector<RE::NiPoint3> positions = {
			a_this->GetPosition(),
			pendingMerge[0]->GetPosition(),
			pendingMerge[1]->GetPosition()
		};

		// only use x and y so Z can stay the sae
		RE::NiPoint3 worldMid{};
		float originalZ = positions[0].z;
		worldMid.z = originalZ;

		float sumX = 0.0f, sumY = 0.0f;
		for (const auto& pos : positions) {
			sumX += pos.x;
			sumY += pos.y;
		}
		worldMid.x = sumX / static_cast<float>(positions.size());
		worldMid.y = sumY / static_cast<float>(positions.size());

		// save original z position
		float originalLocalZ = childLight->local.translate.z;

		// Convert world midpoint to parent's local space
		RE::NiTransform parentWorldTransform = a_root->world;
		RE::NiTransform invTransform = parentWorldTransform.Invert();
		RE::NiPoint3 localMid = invTransform * worldMid;

		// add the z offset 
		localMid.z += originalLocalZ;

		// Logging before
		logger::debug(
			"BEFORE Light {} merge for 3 refs. World XY midpoint = ({}, {}), original Z = {}, local translate = {}",
			childLight->name.c_str(),
			worldMid.x, worldMid.y, originalLocalZ,
			childLight->local.translate
		);

		//we have to use local position its annoying bc its relative to ref a's world position
		childLight->local.translate = localMid;

		//  increase brightness to simulate larger light
		childLight->fade *= 3.0f;

		// gotta update works sometimes without idk why
		if (auto* parent = childLight->parent) {
			RE::NiUpdateData updateData{};
			updateData.time = 0.0f;
			updateData.flags = RE::NiUpdateData::Flag::kDirty;
			parent->UpdateTransformAndBounds(updateData);
		}

		// keep track of merged lights so we dont attach to them later. we reset the set later in event sink
		globals::refsWithAttachedLights.insert(a_this);
		for (auto& ref : pendingMerge) {
			if (ref)
				globals::refsWithAttachedLights.insert(ref);
		}

		// Logging after
		logger::debug(
			"AFTER  Light {} merge for 3 refs. Local translate set = {}, world position approx = {}",
			childLight->name.c_str(),
			childLight->local.translate,
			a_root->world.translate + childLight->local.translate // approximate
		);

		break;
	} default: {
		LightManager::attachLightUsingAttachPath(cfg, a_root, childLight, refFormID);
		break;
	}
  }
}

