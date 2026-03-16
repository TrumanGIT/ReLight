#include "LightManager.h"
#include "Utility.h"
#include "config.hpp"
#include "ClibUtil/EditorID.hpp"
#include "LightAttachmentHooks.h"
#include <chrono>



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
	
	uint32_t mask = cfg.flags; 

	if (mask & static_cast<uint32_t>(LIGHT_FLAGS::kCandle)) {
		bsLight->unk060 = 1; 
	}

	if (mask & static_cast<uint32_t>(LIGHT_FLAGS::kChandelier)) {
		bsLight->unk060 = 2;
	}

	if (mask & static_cast<uint32_t>(LIGHT_FLAGS::kFire)) {
		bsLight->unk060 = 3;
	}
}

bool LightManager::processByFilePath(RE::TESObjectREFR* a_this, RE::NiNode* a_root) {

	if (LightData::meshPathToJsonCfg.empty() && LightData::meshPathToJsonCfgExteriors.empty()) return false;

	const auto baseObject = a_this->GetBaseObject();
	if (!baseObject) return true;
	const auto bm = baseObject->As<RE::TESModel>();
	if (!bm) return true;

	 auto currentModel = std::string(bm->GetModel());

	 auto meshName = extractMeshName(currentModel); 

	auto cell = a_this->GetParentCell();

	if (!cell) {
		logger::warn("no cell cant determine if should use exterior or interior configs");
		return false;
	}

	bool isInterior = cell->IsInteriorCell();

	auto cfgs = findConfigsForMeshPath(meshName, isInterior);

	if (cfgs.empty()) return false; 

	const auto refFormID = a_this->GetFormID();
	const auto baseFormID = baseObject->GetFormID();
	globals::baseFormsWithAttachedLights.emplace(baseFormID);

	logger::debug("file path match found: {}", meshName);

	auto ui = RE::UI::GetSingleton();
	if (ui && ui->IsMenuOpen("InventoryMenu")) {
		return true;
	}

	auto cloneLight = cloneNiPointLight(LightData::masterNiPointLight.light.get());
	if (!cloneLight) {
		logger::warn("Failed to clone NiPointLight for ref {:08X} with mesh '{}' )", refFormID, currentModel);
		return false;
	}

	for (auto& cfg : cfgs) {
		
		LightManager::fillPendingMerges(a_this, cloneLight, cfg, a_root);
		
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
	 { "RuinsFloorCandleLampMidOn", "ruinsfloorcandlelampmidon" },
	 { "RuinsFloorCandleLampMidOn02", "ruinsfloorcandlelampmidon" },
	 { "RuinsFloorCandleLampSmOn", "ruinsfloorcandlelampsmon" },
	 { "RuinsFloorCandleLampSmOn02", "ruinsfloorcandlelampsmon" },

	 { "ImpChandellierCandle01", "chandel" },
	 { "ImpChandellierCandle01USKP", "chandel" },

	 { "CandleLanternwithCandle01", "candle" },

	 { "CandleLanternHandleDown_DynDOLOD_LOD", "candle" },
	 { "CandleLanternwithCandle01_DynDOLOD_LOD.", "candle" },

	 { "ImpChandellierCandle01_DynDOLOD_LOD", "chandel" },

	 { "RuinsFloorCandleLampSmOn_DynDOLOD_LOD", "ruinsfloorcandlelampsmon" },
	 { "RuinsFloorCandleLampMidOn_DynDOLOD_LOD", "ruinsfloorcandlelampmidon" },
	 { "RuinsFloorCandleLampMidOn02_DynDOLOD_LOD", "ruinsfloorcandlelampmidon" },
	 { "RuinsFloorCandleLampSmOn02_DynDOLOD_LOD", "ruinsfloorcandlelampsmon" },
	 };

	 auto baseObject = a_this->GetBaseObject();

	 if (!baseObject) return false;

	 const auto baseFormID = baseObject->GetFormID();

	 const auto bm = baseObject->As<RE::TESModel>();
	 if (!bm) return false;

	 const auto currentModel = bm->GetModel();

	 if (!currentModel) return false; 

	 const auto modelName = extractMeshName(bm->GetModel());

	 logger::debug("dummy found, Model = {}", modelName);

	 auto it = dummyMeshPaths.find(modelName);
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

		 auto cloneLight = cloneNiPointLight(LightData::masterNiPointLight.light.get());

		 if (!cloneLight) {
			 logger::warn("Failed to clone NiPointLight for node '{}', for ref{:08X})", nodeName, refFormID);
			 return true;
		 }

			 LightManager::fillPendingMerges(a_this, cloneLight, cfg, a_root);

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

		auto cloneLight = cloneNiPointLight(LightData::masterNiPointLight.light.get());

		if (!cloneLight) {
			logger::warn("Failed to clone NiPointLight for node '{}' for ref {:08X})", matchStr, refFormID);
			continue;
		}

		/// we merge lights because 2 fxfirewithembers are usually stacked on top of each other, without this double shadow lights (not good)

		LightManager::fillPendingMerges(a_this, cloneLight, cfg, a_root);
	
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

	         // clear so ref can be reprocessed again. for mods like dynamic candles
			globals::refsWithAttachedLights.clear();
		
			// clear so ref can be reprocessed again. for mods like dynamic candles
			globals::mergedRefs.clear();
		
			globals::cellFullyLoaded = true;

		return RE::BSEventNotifyControl::kContinue;
	}

	// player changes from interor to exterior or vice versa, must reinitialize lights
	if (globals::lastCellWasInterior !=	globals::currentCellIsInterior ) {
		
		// stop islightaffectingsurface hook
	//	globals::cellFullyLoaded= false;

		//logger::debug(" new cell detected.. islightaffectingsurface hook stopped");

		//reset wall meshes gathered when going outside since light flicker prevention is not enabled ine exteriors
		if (cell->IsExteriorCell()) globals::wallMeshes.clear();

		LightManager::reinitializeLightsWithinRange(player); 
	}

	// player changes from interor to another interior, must reinitialize otherwise engine cleans the lights
	if (globals::currentCellIsInterior && globals::lastCellWasInterior) {

		// stop islightaffectingsurface hook
		//globals::cellFullyLoaded = false; 

		//logger::debug(" new  interior cell detected.. islightaffectingsurface hook stopped");

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
								logger::warn("attempted to reinitialize light {} but its config ID {} wasent found for ref {:08X}", name, ref->GetFormID(), light->GetLightRuntimeData().unk138);
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
									auto reattachedBSLight = ssNode->AddLight(light, p);

									uint32_t mask = config.flags;

									if (mask & static_cast<uint32_t>(LIGHT_FLAGS::kCandle)) {
										reattachedBSLight->unk060 = 1;
										return;
									}

									if (mask & static_cast<uint32_t>(LIGHT_FLAGS::kChandelier)) {
										reattachedBSLight->unk060 = 2;
										return;
									}

									if (mask & static_cast<uint32_t>(LIGHT_FLAGS::kFire)) {
										reattachedBSLight->unk060 = 3;
									}
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
void LightManager::fillPendingMerges(RE::TESObjectREFR* refA,
	RE::NiPointLight* light, const LightConfig& cfg, RE::NiNode* refA_root) {

	if (!refA || !light) return;

	// attach light to mesh
	LightManager::attachLightUsingAttachPath(cfg, refA_root, light, refA->GetFormID());

	PendingMerge p;

	p.winningConfig = cfg; 

	p.light = RE::NiPointer<RE::NiPointLight>(light); 

	p.refA = refA->GetHandle(); 

	int potentialMergeCount = 0;

	 p.refALightName = cfg.nodeName.empty() ? extractMeshName(cfg.meshPath) : cfg.nodeName;

	bool increasedMergeDistance = false;

	RE::TES::GetSingleton()->ForEachReferenceInRange(refA, globals::lightMergeSeekingDistance, [&](RE::TESObjectREFR* otherRef) {
		if (!otherRef || otherRef == refA) return RE::BSContainer::ForEachResult::kContinue;
		if (potentialMergeCount >= globals::lightMergeMaxLights) return RE::BSContainer::ForEachResult::kStop;

		const RE::FormID refBFormID = otherRef->GetFormID();
		auto base = otherRef->GetBaseObject();
		auto model = base ? base->As<RE::TESModel>() : nullptr;
		if (!model) return RE::BSContainer::ForEachResult::kContinue;

		std::string otherRefName = extractMeshName(model->GetModel());

		//priority grab bc we do partial searches which can bring up false positive matches
		std::string otherRefNameMatch = std::string(findPriorityMatch(otherRefName));

		if (!otherRefNameMatch.empty()) {

			bool looseMatch = false;

			auto cell = otherRef->GetParentCell();
			if (!cell) {
				logger::warn("no cell cant determine if should use exterior or interior configs");
				return RE::BSContainer::ForEachResult::kContinue;
			}

			bool isInterior = cell->IsInteriorCell();
			auto cfgs = findConfigsForNode(otherRefNameMatch, isInterior);

			if (cfgs.empty()) return RE::BSContainer::ForEachResult::kContinue;

			LightConfig otherRefCfg = cfgs[0]; 

			uint32_t refAflags = cfg.flags;

			uint32_t otherRefFlags = otherRefCfg.flags;

			if (refAflags & static_cast<uint32_t>(LIGHT_FLAGS::kCandle)) {

				p.refALightName = "candle";

				if (!increasedMergeDistance && (refAflags & static_cast<uint32_t>(LIGHT_FLAGS::kIncreasedMergeDistance) || otherRefFlags & static_cast<uint32_t>(LIGHT_FLAGS::kIncreasedMergeDistance))) {
					increasedMergeDistance = true;
					logger::debug("increased distance used");
				}
			}

				looseMatch = otherRefNameMatch.find(p.refALightName) != std::string::npos ||
					p.refALightName.find(otherRefNameMatch) != std::string::npos;
			
			//logger::debug("comparing refA {:08X} {} and refB {:08X} {}  for merge == {} distance={}",
			//	refA->GetFormID(), refALightName, refBFormID, otherRefNameMatch, looseMatch, distance);
			
			if (looseMatch && !isExclude(otherRefName, refBFormID)) {

				//the final result of a merged light should  reflect a shadow light if 1 of the emrgies was a shadow light
				if (!cfg.shadowLight && cfgs[0].shadowLight) {
					p.winningConfig = cfgs[0];
				}

				float zDistanceToUse = increasedMergeDistance ? globals::fMaxZDiffToMergeIncreased :  globals::fMaxZDiffToMerge;

				auto distanceToUse = p.winningConfig.shadowLight ? globals::shadowLightMergeDistance : globals::lightMergeDistance;

				// no increased merge distance flag among mergies
				if (!increasedMergeDistance) {

					if (refA->GetDistance(otherRef) > distanceToUse)
						return RE::BSContainer::ForEachResult::kContinue;
				}

				float zDiff = std::abs(refA->GetPosition().z - otherRef->GetPosition().z);
				if (zDiff > zDistanceToUse) {
					logger::debug("refA {:08X} and refB {:08X} z distance {} too great, skipping merge for light {} ",
						refA->GetFormID(), refBFormID, zDiff, p.refALightName, otherRefName);
					return RE::BSContainer::ForEachResult::kContinue;
				}

				p.candidateHandles.push_back(otherRef->GetHandle());
				logger::debug("refA {:08X} and refB {:08X} with matched nodeName {} selected to merge",
					refA->GetFormID(), refBFormID, cfg.nodeName);
				potentialMergeCount++;
			}
		}
		return RE::BSContainer::ForEachResult::kContinue;
		});

	// mark merged refs
	for (const RE::ObjectRefHandle handle : p.candidateHandles) {
		auto ref = handle.get();
		if (ref) globals::mergedRefs.insert(ref.get()->GetFormID());
	}

	if (globals::enableDebugLightBulbs) {
		AttachDebugMarker(refA_root, light);
	}

	if (p.candidateHandles.empty()) {
		LightData::setNiPointLightDataFromCfg(p.light.get(), p.winningConfig);

		p.light->name = "RL" + p.refALightName;
		LightManager::attachNiPointLightToShadowSceneNode(p.light.get(), p.winningConfig, refA);
		return; 
	}

	LightManager::pendingMerges.push_back(p); 

	p.registeredAt = std::chrono::steady_clock::now();
}

 void LightManager::finalizeMerge(PendingMerge& p, std::vector<RE::ObjectRefHandle> validMerges) {

	 auto light = p.light.get(); 

	 if (!light) return; 

	 LightData::setNiPointLightDataFromCfg(p.light.get(), p.winningConfig);

	 p.light->name = "RL" + p.refALightName;

	 auto refA = p.refA.get();
	 if (!refA) return;

	 // no merges so just attach as is
	 if (validMerges.empty()) {
		 LightManager::attachNiPointLightToShadowSceneNode(p.light.get(), p.winningConfig, refA.get());
		 return;
	 }
		 // build positions array: refA first, then all merged refs
		 std::vector<RE::NiPoint3> positions;

		 positions.push_back(refA->GetPosition());
		 for (const RE::ObjectRefHandle refHandle : validMerges) {

			 auto otherRef = refHandle.get(); 

			 if (!otherRef) continue; 
			 positions.push_back(otherRef->GetPosition());
		 }

		 const int mergeCount = static_cast<int>(positions.size());  // includes refA

		 // increasing brightness or radius on meeshes close to each other like fxfirewithemberslgos x fxfirewithembers light 
		 // makes it difficult to configure those cases for users so we dont increase if distance is small
		 bool increaseBrightness = false;

		 for (size_t i = 1; i < positions.size(); i++) {
			 if (positions[0].GetDistance(positions[i]) > 25.0f) {
				 increaseBrightness = true;
				 break;
			 }
		 }

		 //merge count 3 because pending merges includes ref a and we dont want to increase brightness for merges of only 2 light sources
		 if (increaseBrightness && mergeCount >= 3) {
			 float fadeMultiplier = 1.0f + globals::lightFadePerMerge * static_cast<float>(mergeCount - 2);
			 float radiusMultiplier = 1.0f + globals::lightRadiusPerMerge * static_cast<float>(mergeCount - 2);

			 // clamp to max
			 fadeMultiplier = std::min(fadeMultiplier, globals::lightFadeMax);
			 radiusMultiplier = std::min(radiusMultiplier, globals::lightRadiusMax);

			 p.light->fade *= fadeMultiplier;
			 p.light->radius *= radiusMultiplier;
		 }

		 // average X and Y, preserve ref A's Z to apply later
		 float sumX = 0.0f, sumY = 0.0f;
		 for (const auto& pos : positions) {
			 sumX += pos.x;
			 sumY += pos.y;
		 }

		 RE::NiPoint3 worldMid{};
		 worldMid.x = sumX / static_cast<float>(positions.size());
		 worldMid.y = sumY / static_cast<float>(positions.size());
		 worldMid.z = positions[0].z;

		 float originalLocalZ =p.light->local.translate.z;

		 auto* parent = p.light->parent; 

		 if (!parent) {
			 logger::warn("light parent was null cant finish merging light"); 
			 return; 
		 }

		 RE::NiTransform invTransform = parent->world.Invert();
		 RE::NiPoint3 localMid = invTransform * worldMid;
		 localMid.z += originalLocalZ;

		 logger::debug(
			 "[LightMerge:BEFORE] '{}' merging {} refs\n"
			 "  originalLocalZ {}\n"
			 "  local.translate {}",
			 p.light->name.c_str(), mergeCount,
			 originalLocalZ,
			 p.light->local.translate
		 );

		 p.light->local.translate = localMid;


			 RE::NiUpdateData updateData{};
			 updateData.time = 0.0f;
			 updateData.flags = RE::NiUpdateData::Flag::kDirty;
			 parent->UpdateTransformAndBounds(updateData);
		 

		 logger::debug(
			 "[LightMerge:AFTER] '{}' merged {} refs\n"
			 "  local.translate {}",
			 p.light->name.c_str(), mergeCount,
			 p.light->local.translate
		 );
	 

	 LightManager::attachNiPointLightToShadowSceneNode(p.light.get(), p.winningConfig, refA.get());
}


void LightManager::AttachDebugMarker(RE::NiNode* a_node, RE::NiLight* light)
{

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
