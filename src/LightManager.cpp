#include "LightManager.h"
#include "Utility.h"
#include "config.hpp"
#include "ClibUtil/EditorID.hpp"
#include "LightAttachmentHooks.h"

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

	finalNode->AttachChild(light);

	logger::debug("attached light to node {} on ref {:08X} ", finalNodeName, refFormID);
}

// ATTACH NI POINT LIGHT TO SHADOW SCENE NODE TO GET BS LIGHT IN RETURN. BS LIGHT IS THE LIGHT YOU VISUALLY SEE RENDERED IN GAME
//BS LIGHT READS ITS NI POINT LIGHT DATA EVERY FRAME
void LightManager::attachNiPointLightToShadowSceneNode(RE::NiLight* niPointLight, const LightConfig& cfg, RE::TESObjectREFR* a_this) {

	//logger::info("attempting to create NiPointLight BSlight and attach to ShadowSceneNode");

	if (!niPointLight) {
		logger::error("createShadowSceneNode: niPointLight is null");
		return;
	}

	RE::ShadowSceneNode::LIGHT_CREATE_PARAMS params = LightData::makeLightParams(cfg);

	logger::debug("Light paramaters for light {} created for ref {:08X} ", niPointLight->name.c_str(), a_this->GetFormID());

	LightData::printLightParams(params);

	auto* shadowSceneNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];

	if (!shadowSceneNode) {
		logger::warn("no shadow scene node to grab in (createShadowSceneNode()");
		return;
	}

	RE::BSLight* bsLight = shadowSceneNode->AddLight(niPointLight, params);

	if (!bsLight) {
		logger::warn("no BSLight {} created for ref {:08X}", niPointLight->name.c_str(), a_this->GetFormID());
		return;
	}

	//tag so we can find with low overhead in shader hooks
	if (cfg.menuName == "Chandelier") {
		bsLight->unk060 = 1;
		return;
	}

	// TODO:: exclude candlebras 
	if (cfg.menuName.contains("Candle")) {
		bsLight->unk060 = 2;
	}

	// TODO:: exclude candlebras 
	if (cfg.menuName.contains("Fire")) {
		bsLight->unk060 = 3;
	}
}

bool LightManager::processByFilePath(RE::TESObjectREFR* a_this, RE::NiNode* a_root) {

	if (LightData::meshPathToJsonCfg.empty() || LightData::meshPathToJsonCfgExteriors.empty()) return false;

	const auto baseObject = a_this->GetBaseObject();
	if (!baseObject) return true;
	const auto bm = baseObject->As<RE::TESModel>();
	if (!bm) return true;
	 auto currentModel = std::string(bm->GetModel());

	auto cell = a_this->GetParentCell();

	if (!cell) {
		logger::warn("no cell cant determine if should use exterior or interior configs");
		return false;
	}

	bool isInterior = cell->IsInteriorCell();

	auto cfgs = findConfigsForMeshPath(currentModel, isInterior);

	const auto refFormID = a_this->GetFormID();
	const auto baseFormID = baseObject->GetFormID();
	globals::baseFormsWithAttachedLights.emplace(baseFormID);
	logger::debug("file path match found: {}", currentModel);

	auto ui = RE::UI::GetSingleton();
	if (ui && ui->IsMenuOpen("InventoryMenu")) {
		return true;
	}

	auto cloneLight = cloneNiPointLight(PointLight::getMasterPointLight().node.get());
	if (!cloneLight) {
		logger::warn("Failed to clone NiPointLight for ref {:08X} with mesh '{}' )", refFormID, currentModel);
		return false;
	}

	for (auto& cfg : cfgs) {
		if (!cfg.shadowLight) {
			LightManager::attachOrMergeLight(a_this, cloneLight, cfg, a_root, globals::lightMergeDistance);
		}
		else {
			LightManager::attachOrMergeLight(a_this, cloneLight, cfg, a_root, globals::shadowLightMergeDistance);
		}
	}
	return true;
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

	 if (!baseObject) return false;

	 const auto baseFormID = baseObject->GetFormID();

	 const auto bm = baseObject->As<RE::TESModel>();
	 if (!bm) return false;

	 const auto currentModel = bm->GetModel();

	 if (!currentModel) return false; 

	 logger::debug("dummy found, Model = {}", currentModel);

	 auto it = dummyMeshPaths.find(currentModel);
	 if (it != dummyMeshPaths.end()) {

		 auto refFormID = a_this->GetFormID();

		 globals::baseFormsWithAttachedLights.emplace(baseFormID);

		 std::string match = it->second;

		 auto cell = a_this->GetParentCell();

		 if (!cell) {
			 logger::warn("no cell cant determine if should use exterior or interior configs");
			 return false;
		 }

		 bool isInterior = cell->IsInteriorCell();

		 //cant do multi lights currently, just would have to iterate if wanted to.
		 auto cfgs = findConfigsForNode(match, isInterior);

		 if (cfgs.empty()) return false;

		 LightConfig cfg = cfgs[0];

		 auto cloneLight = cloneNiPointLight(PointLight::getMasterPointLight().node.get());

		 if (!cloneLight) {
			 logger::warn("Failed to clone NiPointLight for node '{}', for ref{:08X})", nodeName, refFormID);
			 return true;
		 }

		 if (!cfg.shadowLight) {
			 LightManager::attachOrMergeLight(a_this, cloneLight, cfg, a_root, globals::lightMergeDistance);
		 }

		 else {
			 LightManager::attachOrMergeLight(a_this, cloneLight, cfg, a_root, globals::shadowLightMergeDistance);
		 }

		 logger::debug("dummy node {} with model {} found for ref {:08X} and got light from config: {}", nodeName.c_str(), currentModel, refFormID, cfg.nodeName);
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
		logger::debug(" processing ref {:08X} with node name: {} with baseFormID: {} emplaced in set", refFormID, matchStr, baseFormID);
	}

	if (globals::removeFakeGlowOrbs)
		glowOrbRemover(a_root);

	auto cell = a_this->GetParentCell(); 

	if (!cell) {
		logger::warn("no cell cant determine if should use exterior or interior configs");
		return; 
	}

	bool isInterior = cell->IsInteriorCell(); 

	auto cfgs = findConfigsForNode(matchStr, isInterior);

	//TODO:: what happens if you have a multi light that wants to merge? 
	for (auto& cfg : cfgs) {

		auto cloneLight = cloneNiPointLight(PointLight::getMasterPointLight().node.get());

		if (!cloneLight) {
			logger::warn("Failed to clone NiPointLight for node '{}' for ref {:08X})", matchStr, refFormID);
			continue;
		}

		
/// we merge lights because 2 fxfirewithembers are usually stacked on top of each other, without this double shadow lights (not good)
		if (!cfg.shadowLight) {
			LightManager::attachOrMergeLight(a_this, cloneLight, cfg, a_root, globals::lightMergeDistance);
		}
	
		else {
			LightManager::attachOrMergeLight(a_this, cloneLight, cfg, a_root, globals::shadowLightMergeDistance);
		}
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

	// set the current cell is interior or not
	globals::currentCellIsInterior = cell->IsInteriorCell();

	if (s_firstCellEvent) {
	
		s_firstCellEvent = false;

		logger::info("player is in interior on startup: {}", globals::currentCellIsInterior);
		globals::lastCellWasInterior = cell->IsInteriorCell();

		
		//	std::scoped_lock refsLock(globals::refsWithAttachedLightsMutex);
			globals::refsWithAttachedLights.clear();
		
			//std::scoped_lock mergedLock(globals::mergedRefsMutex);
			globals::mergedRefs.clear();
		

		return RE::BSEventNotifyControl::kContinue;
	}

	// player changes from interor to exterior or vice versa, must reinitialize lights
	if (globals::lastCellWasInterior !=	globals::currentCellIsInterior ) {
		
		//reset wall meshes gathered when going outside since light flicker prevention is not enabled ine exteriors
		if (cell->IsExteriorCell()) globals::wallMeshes.clear();

		LightManager::reinitializeLightsWithinRange(player); 
	}

	// player changes from interor to another interior, must reinitialize otherwise engine cleans the lights
	if (globals::currentCellIsInterior && globals::lastCellWasInterior) {

		LightManager::reinitializeLightsWithinRange(player);
	}

	//set the prev cell after finished evaluating new cell
	globals::lastCellWasInterior = globals::currentCellIsInterior;

	return RE::BSEventNotifyControl::kContinue;
}

void LightManager::registerEventSink()
{
	if (auto* player = RE::PlayerCharacter::GetSingleton()) {
		player->AsBGSActorCellEventSource()->AddEventSink(LightManager::GetSingleton());
		logger::info("BGSActorCellEvent sink registered");
	}
}

void LightManager::reinitializeLightsWithinRange(RE::PlayerCharacter* player) {

	logger::debug("reinitializing lights within range");

	// clear this lights list that has merged lights placed into it so tehey can get lights attached again.
	
		//std::scoped_lock lock(globals::refsWithAttachedLightsMutex);
		globals::refsWithAttachedLights.clear();

		globals::mergedRefs.clear(); 
	 

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

						auto root = ref->Get3D(); 

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
								logger::warn("attempted to reinitialize light but its config ID wasent found for ref {:08X} ", ref->GetFormID() );
								continue;
							}

							const auto& config = it->second;

								auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
								if (!ssNode) {
									logger::warn("ShadowSceneNode[0] is null!");
									continue;
								}
								bool bsLightExists = false;

								for (auto bsLight : ssNode->activeShadowLights) {

									bsLight->worldTranslate;

									if (bsLight->light.get() == light) {
										logger::debug("shadow light {} with ID {} exists already for ref {:08X} skipping reinitialization", light->name, static_cast<void*>(light), ref->GetFormID());
										bsLightExists = true;
										break;
									}
								}

								if (!bsLightExists) {
									logger::debug("reintializing light {} for ref {:08X} is shadow = {}", light->name, ref->GetFormID(), config.shadowLight);

									auto p = LightData::makeLightParams(config);
									ssNode->AddLight(light, p);
								}

							//}
							//regular lights can just disable and renable to reinitialize (light dissapears with the mesh)
							//else {
							//	ref->Disable();
							//	ref->Enable(false);
							//}
						}
					}
				});
			}
		}
		});
	
		//std::scoped_lock refsLock(globals::refsWithAttachedLightsMutex);
		globals::refsWithAttachedLights.clear();
	
	//	std::scoped_lock mergedLock(globals::mergedRefsMutex);
		globals::mergedRefs.clear();
	
}

//used to merge a light with same ref base object within a set distance to help prevent flickering. 
void LightManager::attachOrMergeLight(RE::TESObjectREFR* refA,
	RE::NiPointLight* light, const LightConfig& cfg, RE::NiNode* refA_root, const float radius) {

	if (!refA || !light) return;

	LightManager::attachLightUsingAttachPath(cfg, refA_root, light, refA->GetFormID());

	std::vector<RE::TESObjectREFR*> pendingMerge;
	int potentialMergeCount = 0;

	LightConfig winningConfig = cfg;

	// Find nearby refs with the same base object that haven't been merged yet
	RE::TES::GetSingleton()->ForEachReferenceInRange(refA, radius, [&](RE::TESObjectREFR* otherRef) {
		if (!otherRef || otherRef == refA) return RE::BSContainer::ForEachResult::kContinue;

		if (potentialMergeCount >= 3) return RE::BSContainer::ForEachResult::kStop;

		const RE::FormID refBFormID = otherRef->GetFormID();

		auto base = otherRef->GetBaseObject();
		auto model = base ? base->As<RE::TESModel>() : nullptr;
		if (!model) return RE::BSContainer::ForEachResult::kContinue;

		std::string path = model->GetModel();

		// extract filename without extension
		std::string meshName;
		auto lastSlash = path.find_last_of("/\\");
		auto filename = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
		auto dotPos = filename.find_last_of('.');
		meshName = (dotPos != std::string::npos) ? filename.substr(0, dotPos) : filename;

		// grab name of NiNode (usually 1:1 with mesh names)
		// some nodes have 2 config names in their nodename. for example we need to prioritize candlechangdelier01 to use chandelier lights over candle lights.
		std::string nodeNameMatch = std::string(findPriorityMatch(meshName));

		if (!nodeNameMatch.empty()) {

			bool looseMatch = false;

			if (nodeNameMatch == "candle" || cfg.nodeName == "candle") {
				looseMatch = cfg.nodeName == nodeNameMatch;
			}

			else {
				//problem it works for mesh paths but unintended merges I can forsee happening with mesh path configs
				looseMatch = nodeNameMatch.find(cfg.nodeName) != std::string::npos ||
					cfg.nodeName.find(nodeNameMatch) != std::string::npos;
			}

			if (looseMatch && !isExclude(meshName, refBFormID)) {

				//dont merge lights with z distance greater than... (looks off when doing so) 
				float zDiff = std::abs(refA->GetPosition().z - otherRef->GetPosition().z);
				if (zDiff > globals::fMaxZDiffToMerge) {
					logger::debug(" refA {:08X} and refB {:08X} z distance {} too great, skipping merge for light {}", refA->GetFormID(), refBFormID, zDiff, cfg.nodeName);
					return RE::BSContainer::ForEachResult::kContinue;
				}

				auto cell = otherRef->GetParentCell();

				if (!cell) {
					logger::warn("no cell cant determine if should use exterior or interior configs");
					return RE::BSContainer::ForEachResult::kContinue;
				}

				bool isInterior = cell->IsInteriorCell();

				//refB configs
				auto cfgs = findConfigsForNode(nodeNameMatch, isInterior);

				//if one merged light is a shadow light, merged light should be a shadow.
				//doesent work for multi lights currently 
				if (!cfg.shadowLight && !cfgs.empty() && cfgs[0].shadowLight) {
					winningConfig = cfgs[0];
				}
				pendingMerge.push_back(otherRef);

				logger::debug(" refA {:08X} and refB {:08X} with matched nodeName {} selected to merge ", refA->GetFormID(), refBFormID, cfg.nodeName);
				potentialMergeCount++;
			}
		}

		return RE::BSContainer::ForEachResult::kContinue;
		});

	if (!pendingMerge.empty()) {
		//	std::scoped_lock lock(globals::mergedRefsMutex);
		for (const auto ref : pendingMerge) {
			if (!ref) continue;

			globals::mergedRefs.insert(ref->GetFormID());
		}
	}

	// set data after winning config was determined
	LightData::setNiPointLightDataFromCfg(refA->GetFormID(), light, winningConfig);

	light->name = "RL" + cfg.nodeName;

	// put ref into set so its not prossesssed again


	switch (potentialMergeCount) {

		//1 merge found so place light in between 2 refs. 
	case 1: {
		auto refB = pendingMerge[0];

		if (!refB) {
			logger::warn("no  ref b when merging lights cant merge");
			return;
		}

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
		float originalLocalZ = light->local.translate.z;

		// Convert world midpoint to local space of parent
		RE::NiTransform parentWorldTransform = refA_root->world;
		RE::NiTransform invTransform = parentWorldTransform.Invert();
		RE::NiPoint3 localMid = invTransform * worldMid;

		// Add original Z offset if z level is relatively close
		//if ((refAWorldPos.z - refBWorldPos.z) > globals::mergeZmin)
		localMid.z += originalLocalZ;

		// debug log before change.
		logger::debug(
			"BEFORE Light '{}' merge\n"
			"  refA {:08X} worldPos {}\n"
			"  refB {:08X} worldPos {}\n"
			"distance between 2 refs {}"
			"  originalLocalZ {}\n"
			"  local.translate {}",
			light->name.c_str(),
			refA->GetFormID(), refAWorldPos,
			refB->GetFormID(), refBWorldPos, distance,
			originalLocalZ,
			light->local.translate
		);

		// Apply local position
		light->local.translate = localMid;

		// only multiply if not a small radius merge (2 meshes stacked on top of each oher)
		if (distance > 25) {
			// increase light brightness to simulate larger light
			logger::debug("distance is greater then 10, increasing brightness");
			light->fade *= 1.5f;
		}

		// sometimes have to update the parent for the change to work.
		if (auto* parent = light->parent) {
			RE::NiUpdateData updateData{};
			updateData.time = 0.0f;
			updateData.flags = RE::NiUpdateData::Flag::kDirty;
			parent->UpdateTransformAndBounds(updateData);
		}

		// Logging after change
		logger::debug(
			"[LightMerge:AFTER] '{}'\n"
			"  refA {:08X} worldPos {}\n"
			"  refB {:08X} worldPos {}\n"
			"  local.translate {}",
			light->name.c_str(),
			refA->GetFormID(), refAWorldPos,
			refB->GetFormID(), refBWorldPos,
			light->local.translate
		);

		break;
	}
		  // find the middle of 3 refs and place light in the middle
	case 2: {

		if (!pendingMerge[0] || !pendingMerge[1]) {
			logger::warn("object is null during merge cannot merge");
			return;
		}

		std::vector<RE::NiPoint3> positions = {
			refA->GetPosition(),
			pendingMerge[0]->GetPosition(),
			pendingMerge[1]->GetPosition()
		};


		auto increaseBrightness = false;

		if (positions.empty()) return;

		RE::NiPoint3 refAPos = positions[0];

		for (const auto& pos : positions) {
			const float distance = refAPos.GetDistance(pos);

			if (distance > 25) {

				increaseBrightness = true;
				// increase light brightness to simulate larger light
				logger::debug("distance is greater then 10, increasing brightness");
			}
		}

		if (increaseBrightness) light->fade *= 2.0f;

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
		float originalLocalZ = light->local.translate.z;

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
			light->name.c_str(),
			refA->GetFormID(), positions[0],
			pendingMerge[0]->GetFormID(), positions[1],
			pendingMerge[1]->GetFormID(), positions[2],
			originalLocalZ,
			light->local.translate
		);

		//we have to use local position its annoying bc its relative to ref a's world position
		light->local.translate = localMid;

		// gotta update works sometimes without idk why
		if (auto* parent = light->parent) {
			RE::NiUpdateData updateData{};
			updateData.time = 0.0f;
			updateData.flags = RE::NiUpdateData::Flag::kDirty;
			parent->UpdateTransformAndBounds(updateData);
		}

		logger::debug(
			"[LightMerge:AFTER] '{}'\n"
			"  refA {:08X}\n"
			"  refB {:08X}\n"
			"  refC {:08X}\n"
			"  local.translate {}\n",
			light->name.c_str(),
			refA->GetFormID(),
			pendingMerge[0]->GetFormID(),
			pendingMerge[1]->GetFormID(),
			light->local.translate
		);
		break;

	}
	case 3: {
		if (!pendingMerge[0] || !pendingMerge[1] || !pendingMerge[2]) {
			logger::warn("object is null during merge cannot merge");
			return;
		}
		std::vector<RE::NiPoint3> positions = {
			refA->GetPosition(),
			pendingMerge[0]->GetPosition(),
			pendingMerge[1]->GetPosition(),
			pendingMerge[2]->GetPosition()
		};

		auto increaseBrightness = false;

		if (positions.empty()) return;

		RE::NiPoint3 refAPos = positions[0];

		for (const auto& pos : positions) {
			const float distance = refAPos.GetDistance(pos);

			if (distance > 25) {

				increaseBrightness = true;
				// increase light brightness to simulate larger light
				logger::debug("distance is greater then 10, increasing brightness");
			}
		}

		if (increaseBrightness) light->fade *= 3.5f;

		// only use x and y so Z can stay the same
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
		float originalLocalZ = light->local.translate.z;
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
			"  refD {:08X} pos {}\n"
			"originalLocalZ {}\n"
			"  local.translate {}",
			light->name.c_str(),
			refA->GetFormID(), positions[0],
			pendingMerge[0]->GetFormID(), positions[1],
			pendingMerge[1]->GetFormID(), positions[2],
			pendingMerge[2]->GetFormID(), positions[3],
			originalLocalZ,
			light->local.translate
		);
		// we have to use local position its annoying bc its relative to ref a's world position
		light->local.translate = localMid;

		// gotta update works sometimes without idk why
		if (auto* parent = light->parent) {
			RE::NiUpdateData updateData{};
			updateData.time = 0.0f;
			updateData.flags = RE::NiUpdateData::Flag::kDirty;
			parent->UpdateTransformAndBounds(updateData);
		}
		logger::debug(
			"[LightMerge:AFTER] '{}'\n"
			"  refA {:08X}\n"
			"  refB {:08X}\n"
			"  refC {:08X}\n"
			"  refD {:08X}\n"
			"  local.translate {}\n",
			light->name.c_str(),
			refA->GetFormID(),
			pendingMerge[0]->GetFormID(),
			pendingMerge[1]->GetFormID(),
			pendingMerge[2]->GetFormID(),
			light->local.translate
		);
		break;
	}

	default: {
		break;
	}
	}

	LightManager::attachNiPointLightToShadowSceneNode(light, winningConfig, refA);
}

