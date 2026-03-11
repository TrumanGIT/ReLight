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
			LightManager::attachOrMergeLight(a_this, cloneLight, cfg, a_root, globals::lightMergeSeekingDistance);
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
			 LightManager::attachOrMergeLight(a_this, cloneLight, cfg, a_root, globals::lightMergeSeekingDistance);
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
			LightManager::attachOrMergeLight(a_this, cloneLight, cfg, a_root, globals::lightMergeSeekingDistance);
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

	RE::TES::GetSingleton()->ForEachReferenceInRange(refA, radius, [&](RE::TESObjectREFR* otherRef) {
		if (!otherRef || otherRef == refA) return RE::BSContainer::ForEachResult::kContinue;
		if (potentialMergeCount >= 9) return RE::BSContainer::ForEachResult::kStop;

		const RE::FormID refBFormID = otherRef->GetFormID();
		auto base = otherRef->GetBaseObject();
		auto model = base ? base->As<RE::TESModel>() : nullptr;
		if (!model) return RE::BSContainer::ForEachResult::kContinue;

		std::string otherRefName = extractMeshName(model->GetModel());

		std::string otherRefNameMatch = std::string(findPriorityMatch(otherRefName));

		if (!otherRefNameMatch.empty()) {

			std::string refALightName = "";

			// if node name emtpy check mesh path as a identifier instead
			cfg.nodeName.empty() ? refALightName = extractMeshName(cfg.meshPath) : refALightName = cfg.nodeName;

			float zDistanceToUse = globals::fMaxZDiffToMerge;
			bool processingRuinCandle = false;

			if (refALightName.contains("candle") || otherRefName.contains("candle")) {

				// ruin candels are aggretious light flickerer casuses and must be merged exxessivly.
				if (refALightName.contains("ruin") || otherRefName.contains("ruin")) {
					zDistanceToUse = globals::fMaxZDiffToMergeIncreased;
					processingRuinCandle = true; 
					logger::debug("increased distance used");
				}

				if (!refALightName.contains("chande") && !refALightName.contains("bra")) {
					refALightName = "candle";
				}
			}

			bool looseMatch = false;
		
			//if ref a model name contains ref b model name or vice versa, they should merge
				looseMatch = otherRefNameMatch.find(refALightName) != std::string::npos ||
				refALightName.find(otherRefNameMatch) != std::string::npos;

				logger::debug("comparing refA {:08X} {} and refB {:08X} {}  for merge == {}",
					refA->GetFormID(), refALightName, refBFormID, otherRefNameMatch, looseMatch);

			if (looseMatch && !isExclude(otherRefName, refBFormID)) {
				float zDiff = std::abs(refA->GetPosition().z - otherRef->GetPosition().z);
				if (zDiff > zDistanceToUse) {
					logger::debug("refA {:08X} and refB {:08X} z distance {} too great, skipping merge for light {}",
						refA->GetFormID(), refBFormID, zDiff, cfg.nodeName);
					return RE::BSContainer::ForEachResult::kContinue;
				}

				auto cell = otherRef->GetParentCell();
				if (!cell) {
					logger::warn("no cell cant determine if should use exterior or interior configs");
					return RE::BSContainer::ForEachResult::kContinue;
				}

				bool isInterior = cell->IsInteriorCell();
				auto cfgs = findConfigsForNode(otherRefNameMatch, isInterior);

				if (cfgs.empty()) return RE::BSContainer::ForEachResult::kContinue;

				//the final result of a merged light should  reflect a shadow light if 1 of the emrgies was a shadow light
				if (!cfg.shadowLight && cfgs[0].shadowLight) {
					winningConfig = cfgs[0];
				}

				if (!cfg.shadowLight && !cfgs[0].shadowLight && processingRuinCandle == false) {
				if (refA->GetDistance(otherRef) > globals::lightMergeDistance)
					return RE::BSContainer::ForEachResult::kContinue;
				}

				pendingMerge.push_back(otherRef);
				logger::debug("refA {:08X} and refB {:08X} with matched nodeName {} selected to merge",
					refA->GetFormID(), refBFormID, cfg.nodeName);
				potentialMergeCount++;
			}
		}
		return RE::BSContainer::ForEachResult::kContinue;
		});

	// mark merged refs
	for (const auto ref : pendingMerge) {
		if (ref) globals::mergedRefs.insert(ref->GetFormID());
	}

	LightData::setNiPointLightDataFromCfg(refA->GetFormID(), light, winningConfig);
	light->name = "RL" + cfg.nodeName;

	if (globals::enableDebugLightBulbs) 	AttachDebugMarker(refA_root, light);

	// handle merge positioning
	if (!pendingMerge.empty()) {
		// build positions array: refA first, then all merged refs
		std::vector<RE::NiPoint3> positions;
		positions.push_back(refA->GetPosition());
		for (const auto ref : pendingMerge) {
			positions.push_back(ref->GetPosition());
		}

		const int mergeCount = static_cast<int>(positions.size()); // includes refA

		// check if any ref is far enough to warrant brightness increase
		bool increaseBrightness = false;
		for (size_t i = 1; i < positions.size(); i++) {
			if (positions[0].GetDistance(positions[i]) > 25.0f) {
				increaseBrightness = true;
				break;
			}
		}

		//TODO shouldent include fxfirewiwthembers lgos or the wood one
		if (increaseBrightness) {
			// scale brightness based on merge count
			light->fade *= (1.0f + (0.3f * static_cast<float>(mergeCount - 1)));
			light->radius *= (1.0f + (0.3f * static_cast<float>(mergeCount - 1)));
		}

		// average X and Y, preserve refA's Z
		float sumX = 0.0f, sumY = 0.0f;
		for (const auto& pos : positions) {
			sumX += pos.x;
			sumY += pos.y;
		}

		RE::NiPoint3 worldMid{};
		worldMid.x = sumX / static_cast<float>(positions.size());
		worldMid.y = sumY / static_cast<float>(positions.size());
		worldMid.z = positions[0].z;

		float originalLocalZ = light->local.translate.z;


		RE::NiTransform invTransform = refA_root->world.Invert();
		RE::NiPoint3 localMid = invTransform * worldMid;
		localMid.z += originalLocalZ;

		logger::debug(
			"[LightMerge:BEFORE] '{}' merging {} refs\n"
			"  originalLocalZ {}\n"
			"  local.translate {}",
			light->name.c_str(), mergeCount,
			originalLocalZ,
			light->local.translate
		);

		light->local.translate = localMid;

		if (auto* parent = light->parent) {
			RE::NiUpdateData updateData{};
			updateData.time = 0.0f;
			updateData.flags = RE::NiUpdateData::Flag::kDirty;
			parent->UpdateTransformAndBounds(updateData);
		}

		logger::debug(
			"[LightMerge:AFTER] '{}' merged {} refs\n"
			"  local.translate {}",
			light->name.c_str(), mergeCount,
			light->local.translate
		);
	}

	LightManager::attachNiPointLightToShadowSceneNode(light, winningConfig, refA);
}

void LightManager::AttachDebugMarker(RE::NiNode* a_node, RE::NiLight* light)
{
	/*if (!Settings::GetSingleton()->LoadDebugMarkers()) {
		return nullptr;
	}*/

	RE::NiPointer<RE::NiNode>                              loadedModel;
	constexpr RE::BSModelDB::DBTraits::ArgsType args{};

	if (const auto error = Demand("marker_light.nif", loadedModel, args); error == RE::BSResource::ErrorCode::kNone) {
		if (const auto clonedModel = loadedModel->Clone()) {
			loadedModel.reset();
			auto clonedModelAsNode = clonedModel->AsNode(); 

			if (!clonedModelAsNode) {
				logger::debug("could not case debug marker as node, skipping debug marker");
				return;
			}

			a_node->AttachChild(clonedModelAsNode);

			clonedModelAsNode->local.translate = light->local.translate; 
		}
		else { logger::debug("clone failed for debug light marker"); }

	}

}
