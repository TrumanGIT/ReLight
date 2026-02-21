#include "LightManager.h"
#include "Utility.h"
#include "config.hpp"
#include "ClibUtil/EditorID.hpp"

//ATTACH LIGHTS AT CORRECT MESH INDEX, USEFULL FOR TORCHES WHERE LIGHT MUST BE INSERTED TO SPECIFIC SPOT
void LightManager::attachLightUsingAttachPath(
	const LightConfig& cfg,
	RE::NiNode* root,
	RE::NiPointLight* light, RE::FormID refFormID)
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
void LightManager::attachNiPointLightToShadowSceneNode(RE::NiLight* niPointLight, const LightConfig& cfg, RE::FormID refFormID) {

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

	const auto currentModel = bm->GetModel();

	for (auto& [meshPath, cfgs] : LightData::meshPathToJsonCfg) {

		if (meshPath != currentModel) continue;

	const auto refFormID = a_this->GetFormID(); 

	  auto shadowLightFound = false;

	  const auto baseFormID = baseObject->GetFormID();

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

		for (auto& cfg : cfgs) {

		LightData::setNiPointLightDataFromCfg(refFormID, cloneLight, cfg);

		std::string temp = "RL" + cfg.nodeName;
		cloneLight->name = temp.c_str();

		if (!cfg.shadowLight) {
			LightManager::attachOrMergeLight(a_this, cfg.nodeName, cloneLight, cfg, a_root, globals::lightMergeDistance, shadowLightFound);
		}

		else {
			LightManager::attachOrMergeLight(a_this, cfg.nodeName, cloneLight, cfg, a_root, globals::shadowLightMergeDistance, shadowLightFound);
		}

		//if one ref was a shadow light, the merged light should be aswell
		if (shadowLightFound && !cfg.shadowLight) {
			cfg.shadowLight = true;
		}

		LightManager::attachNiPointLightToShadowSceneNode(cloneLight, cfg, a_this->GetFormID());

		}

		return true;
	}
	return false;
}

 //TODO:: doesent handle multi lights in a single config (idk if it needs to really) 
// some nodes are called dummy this is to take care of them.
 bool LightManager::dummyHandler(RE::TESObjectREFR* a_this, const RE::BSFixedString& nodeName, RE::NiNode* a_root)
{

	if (!nodeName.contains("dummy")) return false;

	logger::debug("dummy found");

	//TODO:: this only works if the node name in the json config is labled exactly as matched below. 
	// should probly cover cases where the user changed the node name from chandel to something else 
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

		auto shadowLightFound = false;
		auto shadowRadiiMult = 1.0; 

		globals::baseFormsWithAttachedLights.emplace(baseFormID);
		
		 std::string match = it->second;

		 //cant do multi lights currently, just would have to iterate if wanted to.
		 auto cfg = findConfigsForNode(match)[0];

		auto cloneLight = cloneNiPointLight(PointLight::getMasterPointLight().node.get());

		if (!cloneLight) {
			logger::warn("Failed to clone NiPointLight for node '{}')", nodeName);
			return true;
		}

		LightData::setNiPointLightDataFromCfg(refFormID, cloneLight, cfg);

		cloneLight->name = "RL" + cfg.nodeName;

		if (!cfg.shadowLight) {
			LightManager::attachOrMergeLight(a_this, cfg.nodeName, cloneLight, cfg, a_root, globals::lightMergeDistance, shadowLightFound);
		}

		else {
			LightManager::attachOrMergeLight(a_this, cfg.nodeName, cloneLight, cfg, a_root, globals::shadowLightMergeDistance, shadowLightFound);
		}

		// if one is a shadow light then the merged light should be a shadow light
	//one of the merged lights was a shadow light.
		if (shadowLightFound && !cfg.shadowLight) {
			cfg.shadowLight = true;
		}


		LightManager::attachNiPointLightToShadowSceneNode(cloneLight, cfg, refFormID);

		logger::debug("dummy node {} with model {} found for ref {} and got light from config: {}", nodeName.c_str(), currentModel, a_this->GetFormID(), cfg.nodeName );
		return true;
	}

	// already returned early if not a dummy, therefor might as well skip this object as it wouldent get light anyway
	return true;
}


 void LightManager::processByNodeName(RE::NiNode* a_root, const RE::BSFixedString& match, RE::TESObjectREFR* a_this) {

	// matched name with a configs nodename
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

	//TODO:: what happens if you have a multi light that wants to merge? 
	for (auto& cfg : cfgs) {
		auto cloneLight = cloneNiPointLight(PointLight::getMasterPointLight().node.get());

		if (!cloneLight) {
			logger::warn("Failed to clone NiPointLight for node '{}')", matchStr);
			continue;
		}

		LightData::setNiPointLightDataFromCfg( refFormID, cloneLight, cfg);

		cloneLight->name = "RL" + cfg.nodeName;

		auto shadowLightFound = false;

		/// we merge lights because 2 fxfirewithembers are usually stacked on top of each other, without this double shadow lights (not good)
		if (!cfg.shadowLight) {
			LightManager::attachOrMergeLight(a_this, cfg.nodeName, cloneLight, cfg, a_root, globals::lightMergeDistance, shadowLightFound);
		}
	
		else {
			LightManager::attachOrMergeLight(a_this, cfg.nodeName, cloneLight, cfg, a_root, globals::shadowLightMergeDistance, shadowLightFound);
		}

		//one of the merged lights was a shadow light.
		if (shadowLightFound && !cfg.shadowLight) {
			cfg.shadowLight = true;
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

		logger::debug("Users object brightness ini setting = {}", fLODFadeOutMultObjects);

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
	
	RE::TES::GetSingleton()->ForEachReferenceInRange(player, globals::fLODFadeOutMultObjects, [](RE::TESObjectREFR* ref) {

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
									logger::info("reintializing shadow light {} for ref {} ", light->name, ref->GetFormID());

									auto p = LightData::makeLightParams(config);
									ssNode->AddLight(light, p);
								}

							}
							//regular lights can just disable and renable to reinitialize (light dissapears with the mesh)
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
void LightManager::attachOrMergeLight(RE::TESObjectREFR* refA, const std::string& nodeName,
	RE::NiPointLight* childLight, const LightConfig& cfg, RE::NiNode* refA_root, const float radius, bool shadowLightFound)
{
	if (!refA || !childLight) return;

	std::vector<RE::TESObjectREFR*> pendingMerge;
	int potentialMergeCount = 0;

	// Find nearby refs with the same base object that haven't been merged yet
	RE::TES::GetSingleton()->ForEachReferenceInRange(refA, radius, [&](RE::TESObjectREFR* refB) {
		if (refB == refA) return RE::BSContainer::ForEachResult::kContinue;

		//skip refs that already have lights.
		if (globals::refsWithAttachedLights.count(refB) == 0) {
			
			// could prolly pass thhis in from earlier
			auto baseObj = refB->GetBaseObject();

		    const RE::FormID refBFormID = refB->GetFormID();

			//call false so it skips our hook
			auto niAVObject = refB->Load3D(false);

			if (!niAVObject) {
				logger::warn("no ni node casted from niav object from lwhen merging hook");
				return RE::BSContainer::ForEachResult::kContinue;
			}

			//helps filter out a few things we dont want to touch (fog, mist)
			auto refB_root = niAVObject->AsNode();
			if (!refB_root) {
				logger::warn("no ni node casted from niav object during merge");
				return RE::BSContainer::ForEachResult::kContinue;
			}
			// grab name of NiNode (usually 1:1 with mesh names)

			// some nodes have 2 config names in their nodename. for example we need to prioritize candlechangdelier01 to use chandelier lights over candle lights.
			 std::string nodeNameMatch = std::string(findPriorityMatch(refB_root->name));

			if (!nodeNameMatch.empty()) {

				// if refA's matched nodename and ref Bs matched node name are the same, then merge (its a loose match system that works for now)
				if (nodeName == nodeNameMatch && !isExclude(refB_root->name, refB_root, refBFormID)) {

				auto cfgs =	findConfigsForNode(nodeNameMatch);

				//if one merged light is a shadow light, merged light should be a shadow.
				//doesent work for multi lights currently
				if (cfgs[0].shadowLight) {
					shadowLightFound = true;
				}

					pendingMerge.push_back(refB);
					logger::debug("refA {} and refB {} with matched nodeName {} selected to merge ", refA->GetFormID(), refBFormID, nodeName);
					potentialMergeCount++;
				}
			}
		}
		return RE::BSContainer::ForEachResult::kContinue;
		});

	switch (potentialMergeCount) {

		//1 merge found so place light in between 2 refs. 
	case 1: {

		LightManager::attachLightUsingAttachPath(cfg, refA_root, childLight, refA->GetFormID());

		auto refB = pendingMerge[0];

		// Get world positions of the references we want the lgiht to go in between
		const RE::NiPoint3 refAWorldPos = refA->GetPosition();
		const RE::NiPoint3 refBWorldPos = refB->GetPosition();

		const float distance = refAWorldPos.GetDistance(refBWorldPos);

		// find mid point of 2 refs to palce light in between (x y) only
		RE::NiPoint3 worldMid{};
		worldMid.x = (refAWorldPos.x + refBWorldPos.x) * 0.5f;
		worldMid.y = (refAWorldPos.y + refBWorldPos.y) * 0.5f;

		// Preserve original Z as offset
		worldMid.z = refAWorldPos.z;

		// Save original local Z so we can apply it again later
		float originalLocalZ = childLight->local.translate.z;

		// Convert world midpoint to local space of parent
		RE::NiTransform parentWorldTransform = refA_root->world;
		RE::NiTransform invTransform = parentWorldTransform.Invert();
		RE::NiPoint3 localMid = invTransform * worldMid;

		// Add original Z offset
		localMid.z += originalLocalZ;

		// debug log before change.
		logger::debug(
			"BEFORE Light '{}' merge\n"
			"  refA {:08X} worldPos {}\n"
			"  refB {:08X} worldPos {}\n"
			"distance between 2 refs {}"
			"  originalLocalZ {}\n"
			"  local.translate {}",
			childLight->name.c_str(),
			refA->GetFormID(), refAWorldPos,
			refB->GetFormID(), refBWorldPos, distance,
			originalLocalZ,
			childLight->local.translate
		);

		// Apply local position
		childLight->local.translate = localMid;

		// only multiply if not a small radius merge (2 meshes stacked on top of each oher)
		if (distance > 25) {
			// increase light brightness to simulate larger light
			logger::debug("distance is greater then 10, increasing brightness");
			childLight->fade *= 2.0f;
		}	

		// sometimes have to update the parent for the change to work.
		if (auto* parent = childLight->parent) {
			RE::NiUpdateData updateData{};
			updateData.time = 0.0f;
			updateData.flags = RE::NiUpdateData::Flag::kDirty;
			parent->UpdateTransformAndBounds(updateData);
		}

		// Track references with attached lights so we dont double attach lights to them.
		globals::refsWithAttachedLights.insert(refA);
		globals::refsWithAttachedLights.insert(refB);

		// Logging after change
		logger::debug(
			"[LightMerge:AFTER] '{}'\n"
			"  refA {:08X} worldPos {}\n"
			"  refB {:08X} worldPos {}\n"
			"  local.translate {}",
			childLight->name.c_str(),
			refA->GetFormID(), refAWorldPos,
			refB->GetFormID(), refBWorldPos,
			childLight->local.translate
		);

		break;
	}
		  // find the middle of 3 refs and place light in the middle
	case 2: {

		LightManager::attachLightUsingAttachPath(cfg, refA_root, childLight, refA->GetFormID());

		std::vector<RE::NiPoint3> positions = {
			refA->GetPosition(),
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
		RE::NiTransform parentWorldTransform = refA_root->world;
		RE::NiTransform invTransform = parentWorldTransform.Invert();
		RE::NiPoint3 localMid = invTransform * worldMid;

		// add the z offset 
		localMid.z += originalLocalZ;

		logger::debug(
			"[LightMerge:BEFORE] '{}'\n"
			"  refA {:08X} pos {}\n"
			"  refB {:08X} pos {}\n"
			"  refC {:08X} pos {}\n"
			"originalLocalZ {}\n"
			"  local.translate {}",
			childLight->name.c_str(),
			refA->GetFormID(), positions[0],
			pendingMerge[0]->GetFormID(), positions[1],
			pendingMerge[1]->GetFormID(), positions[2],
			originalLocalZ,
			childLight->local.translate
		);

		//we have to use local position its annoying bc its relative to ref a's world position
		childLight->local.translate = localMid;

		   // always increase radius for 3
			childLight->fade *= 3.0f;
		
		// gotta update works sometimes without idk why
		if (auto* parent = childLight->parent) {
			RE::NiUpdateData updateData{};
			updateData.time = 0.0f;
			updateData.flags = RE::NiUpdateData::Flag::kDirty;
			parent->UpdateTransformAndBounds(updateData);
		}

		// keep track of merged lights so we dont attach to them later. we reset the set later in event sink
		globals::refsWithAttachedLights.insert(refA);

		for (auto& ref : pendingMerge) {
			if (ref)
				globals::refsWithAttachedLights.insert(ref);
		}

		logger::debug(
			"[LightMerge:AFTER] '{}'\n"
			"  refA {:08X}\n"
			"  refB {:08X}\n"
			"  refC {:08X}\n"
			"  local.translate {}\n",
			childLight->name.c_str(),
			refA->GetFormID(),
			pendingMerge[0]->GetFormID(),
			pendingMerge[1]->GetFormID(),
			childLight->local.translate
		);

		break;

	} default: {
		LightManager::attachLightUsingAttachPath(cfg, refA_root, childLight, refA->GetFormID());
		break;
	}
  }
}

